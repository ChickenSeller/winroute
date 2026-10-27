#include "WinDivertInterceptor.h"

#include "FakeDns.h"      // DnsCache
#include "FlowTable.h"
#include "Logger.h"
#include "ProcessInfo.h"
#include "Utils.h"
#include "WinDivertApi.h"
#include "WinDivertPacket.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>

#include <cstring>
#include <vector>

namespace {

// WinDivert socket/flow-layer ports are host byte order; packet addresses (the
// NETWORK layer) are network byte order and read as raw bytes by wdpkt, so all
// address handling below is endian-safe. Only the relay redirect's inject
// direction (Outbound/Loopback) is validated at run time (see networkLoop).

QHostAddress hostFromBytes(const quint8 *b, bool v6)
{
    if (v6)
        return util::normalize(QHostAddress(b));
    const quint32 h = (quint32(b[0]) << 24) | (quint32(b[1]) << 16) | (quint32(b[2]) << 8) | b[3];
    return QHostAddress(h);
}

// State of the "WinDivert" kernel service, or 0 if it does not exist.
DWORD winDivertServiceState()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return 0;
    DWORD state = 0;
    if (SC_HANDLE svc = OpenServiceW(scm, L"WinDivert", SERVICE_QUERY_STATUS)) {
        SERVICE_STATUS st{};
        if (QueryServiceStatus(svc, &st))
            state = st.dwCurrentState;
        CloseServiceHandle(svc);
    }
    CloseServiceHandle(scm);
    return state;
}

} // namespace

WinDivertInterceptor::WinDivertInterceptor()
{
    m_selfPid = GetCurrentProcessId();
    m_selfSession = procinfo::currentSessionId();
}

WinDivertInterceptor::~WinDivertInterceptor()
{
    stop();
}

bool WinDivertInterceptor::available()
{
    return WinDivertApi::instance()->loaded() && !WinDivertApi::driverPath().isEmpty();
}

bool WinDivertInterceptor::serviceRunning()
{
    return winDivertServiceState() == SERVICE_RUNNING;
}

WinDivertInterceptor::Info WinDivertInterceptor::info()
{
    Info info;
    info.available = available();
    if (!info.available)
        return info;
    info.running = serviceRunning();
    const QString sys = WinDivertApi::driverPath();
    info.version = codesign::fileVersion(sys);
    info.certs = codesign::fileSigners(sys);
    return info;
}

bool WinDivertInterceptor::uninstall(QString *error)
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        if (error)
            *error = QCoreApplication::translate("Interceptor", "Cannot open the service manager (error %1)").arg(GetLastError());
        return false;
    }
    SC_HANDLE svc = OpenServiceW(scm, L"WinDivert", SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (!svc) {
        const DWORD err = GetLastError();
        CloseServiceHandle(scm);
        if (err == ERROR_SERVICE_DOES_NOT_EXIST)
            return true;   // nothing to remove
        if (error)
            *error = QCoreApplication::translate("Interceptor", "Cannot open the WinDivert service (error %1)").arg(err);
        return false;
    }
    SERVICE_STATUS st{};
    if (QueryServiceStatus(svc, &st) && st.dwCurrentState != SERVICE_STOPPED)
        ControlService(svc, SERVICE_CONTROL_STOP, &st);
    const bool deleted = DeleteService(svc) || GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE
                         || GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
    const DWORD err = GetLastError();
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    if (!deleted && error)
        *error = QCoreApplication::translate("Interceptor", "Cannot delete the WinDivert service (error %1)").arg(err);
    return deleted;
}

bool WinDivertInterceptor::start(const InterceptorContext &ctx, QString *error)
{
    const WinDivertApi *api = WinDivertApi::instance();
    if (!api->loaded()) {
        if (error)
            *error = QCoreApplication::translate("Interceptor", "WinDivert.dll could not be loaded; is it next to WinRoute.exe?");
        return false;
    }
    m_ctx = ctx;
    m_relayPort = ctx.relayPort;

    const std::string socketFilter = "tcp and (event == CONNECT or event == CLOSE)";
    const std::string netFilter =
        "tcp and not impostor and ((outbound and not loopback) or (loopback and (tcp.SrcPort == "
        + std::to_string(m_relayPort) + " or tcp.DstPort == " + std::to_string(m_relayPort) + ")))";

    m_socketHandle = api->open(socketFilter.c_str(), WINDIVERT_LAYER_SOCKET, 0,
                               WINDIVERT_FLAG_SNIFF | WINDIVERT_FLAG_RECV_ONLY);
    if (m_socketHandle == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        if (error)
            *error = QCoreApplication::translate("Interceptor",
                         "WinDivert could not open the socket layer (error %1). Administrator rights are required.").arg(err);
        return false;
    }
    m_networkHandle = api->open(netFilter.c_str(), WINDIVERT_LAYER_NETWORK, -100, 0);
    if (m_networkHandle == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        api->close(m_socketHandle);
        m_socketHandle = INVALID_HANDLE_VALUE;
        if (error)
            *error = QCoreApplication::translate("Interceptor",
                         "WinDivert could not open the network layer (error %1).").arg(err);
        return false;
    }

    {
        QMutexLocker lock(&m_connMutex);
        m_conns.clear();
    }
    m_running = true;
    m_socketThread = std::thread([this] { socketLoop(); });
    m_networkThread = std::thread([this] { networkLoop(); });
    return true;
}

void WinDivertInterceptor::stop()
{
    if (!m_running.exchange(false))
        return;
    const WinDivertApi *api = WinDivertApi::instance();
    // Unblock the recv loops, then close.
    if (m_socketHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_socketHandle, WINDIVERT_SHUTDOWN_BOTH);
    if (m_networkHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_networkHandle, WINDIVERT_SHUTDOWN_BOTH);
    if (m_socketThread.joinable())
        m_socketThread.join();
    if (m_networkThread.joinable())
        m_networkThread.join();
    if (m_socketHandle != INVALID_HANDLE_VALUE) {
        api->close(m_socketHandle);
        m_socketHandle = INVALID_HANDLE_VALUE;
    }
    if (m_networkHandle != INVALID_HANDLE_VALUE) {
        api->close(m_networkHandle);
        m_networkHandle = INVALID_HANDLE_VALUE;
    }
    QMutexLocker lock(&m_connMutex);
    m_conns.clear();
}

void WinDivertInterceptor::setRules(RuleEnginePtr rules)
{
    QSet<QString> endpoints;
    if (rules) {
        for (const ProxyServer &p : rules->profile().proxies) {
            const QHostAddress literal(p.address);
            if (!literal.isNull())
                endpoints.insert(util::formatEndpoint(util::normalize(literal), p.port));
        }
    }
    QMutexLocker lock(&m_rulesMutex);
    m_rules = std::move(rules);
    m_proxyEndpoints = endpoints;
}

RuleEnginePtr WinDivertInterceptor::rules() const
{
    QMutexLocker lock(&m_rulesMutex);
    return m_rules;
}

void WinDivertInterceptor::socketLoop()
{
    const WinDivertApi *api = WinDivertApi::instance();
    WINDIVERT_ADDRESS addr;
    while (m_running) {
        if (!api->recv(m_socketHandle, nullptr, 0, nullptr, &addr)) {
            if (!m_running)
                break;
            continue;
        }
        const quint32 pid = addr.Socket.ProcessId;
        const quint16 localPort = addr.Socket.LocalPort;
        if (pid == m_selfPid || localPort == 0)
            continue;
        if (addr.Event == WINDIVERT_EVENT_SOCKET_CLOSE) {
            QMutexLocker lock(&m_connMutex);
            m_conns.remove(localPort);
            continue;
        }
        if (addr.Event != WINDIVERT_EVENT_SOCKET_CONNECT)
            continue;
        procinfo::Info pinfo = procinfo::lookup(pid);
        if (!pinfo.path.isEmpty())
            pinfo.name = QFileInfo(pinfo.path).fileName();
        Conn c;
        c.state = State::Pending;
        c.pid = pid;
        c.appName = pinfo.name;
        c.appPath = pinfo.path;
        c.sessionId = pinfo.sessionId;
        c.v6 = addr.IPv6;
        QMutexLocker lock(&m_connMutex);
        m_conns.insert(localPort, c);   // a new connect replaces any stale entry on this port
    }
}

void WinDivertInterceptor::decide(Conn &c, const QHostAddress &dst, quint16 dport)
{
    const RuleEnginePtr engine = rules();
    if (!engine) {
        c.state = State::Passthrough;
        return;
    }
    const ProfileOptions &opt = engine->profile().options;

    const bool isService = c.pid == 4 || c.sessionId == 0;
    if ((isService && !opt.processServices) || (!isService && c.sessionId != m_selfSession && !opt.processOtherUsers)) {
        c.state = State::Passthrough;
        return;
    }

    const QString host = m_ctx.dnsCache ? m_ctx.dnsCache->lookup(dst) : QString();
    ConnectionRequest req;
    req.appPath = c.appPath;
    req.appName = c.appName;
    req.pid = c.pid;
    req.ip = dst;
    req.hostname = host;
    req.port = dport;
    const RuleMatch match = engine->match(req);
    const QString target = host.isEmpty() ? util::formatEndpoint(dst, dport) : QString("%1:%2").arg(host).arg(dport);

    switch (match.action.type) {
    case ActionType::Block:
        c.state = State::Blocked;
        logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : blocked").arg(c.appName, target, match.ruleName));
        return;
    case ActionType::Direct:
        if (!opt.handleDirectConnections) {
            c.state = State::Passthrough;
            logVerbose(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : direct connection").arg(c.appName, target, match.ruleName));
            return;
        }
        break;
    case ActionType::Proxy:
    case ActionType::Chain:
        if (opt.loopDetection) {
            QMutexLocker lock(&m_rulesMutex);
            if (m_proxyEndpoints.contains(util::formatEndpoint(dst, dport))) {
                lock.unlock();
                c.state = State::Passthrough;
                logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 is a proxy server; connecting directly to prevent a connection loop").arg(c.appName, target));
                return;
            }
        }
        break;
    }

    c.state = State::Active;
    c.realPort = dport;
    c.hostname = host;
    c.match = match;
}

void WinDivertInterceptor::networkLoop()
{
    const WinDivertApi *api = WinDivertApi::instance();
    std::vector<unsigned char> buf(WINDIVERT_MTU_MAX);
    WINDIVERT_ADDRESS addr;
    UINT len = 0;

    while (m_running) {
        if (!api->recv(m_networkHandle, buf.data(), UINT(buf.size()), &len, &addr)) {
            if (!m_running)
                break;
            continue;
        }
        wdpkt::Endpoints ep;
        if (!wdpkt::readTcp(buf.data(), len, &ep)) {
            api->send(m_networkHandle, buf.data(), len, nullptr, &addr);   // not TCP we can parse; pass through
            continue;
        }

        bool rewritten = false;
        bool drop = false;

        if (addr.Outbound && !addr.Loopback) {
            // Application -> server. Redirect the proxied ones to the relay.
            QMutexLocker lock(&m_connMutex);
            auto it = m_conns.find(ep.srcPort);
            if (it != m_conns.end()) {
                if (it->state == State::Pending) {
                    const QHostAddress dst = hostFromBytes(ep.dst, ep.v6);
                    decide(*it, dst, ep.dstPort);
                    if (it->state == State::Active) {
                        std::memcpy(it->realDst, ep.dst, 16);
                        it->v6 = ep.v6;
                        RedirectInfo info;
                        info.pid = it->pid;
                        info.appPath = it->appPath;
                        info.appName = it->appName;
                        info.srcAddr = hostFromBytes(ep.src, ep.v6);
                        info.srcPort = ep.srcPort;
                        info.dstAddr = dst;
                        info.dstPort = ep.dstPort;
                        info.hostname = it->hostname;
                        info.match = it->match;
                        info.createdMs = QDateTime::currentMSecsSinceEpoch();
                        if (m_ctx.flows)
                            m_ctx.flows->add(info);
                    }
                }
                switch (it->state) {
                case State::Active:
                    // Self-redirect: destination becomes the source IP, relay port;
                    // the source port is preserved so the relay can match the flow.
                    lock.unlock();
                    wdpkt::setTcpDst(buf.data(), len, ep.src, m_relayPort);
                    addr.Outbound = 0;   // deliver inbound to the local relay
                    addr.Loopback = 1;
                    rewritten = true;
                    break;
                case State::Blocked:
                    drop = true;
                    break;
                default:   // Passthrough / Pending-not-ours
                    break;
                }
            }
        } else if (addr.Loopback && ep.srcPort == m_relayPort) {
            // Relay -> application. Restore the original server as the source.
            QMutexLocker lock(&m_connMutex);
            auto it = m_conns.find(ep.dstPort);
            if (it != m_conns.end() && it->state == State::Active) {
                quint8 dstCopy[16];
                std::memcpy(dstCopy, it->realDst, 16);
                const quint16 realPort = it->realPort;
                lock.unlock();
                wdpkt::setTcpSrc(buf.data(), len, dstCopy, realPort);
                addr.Outbound = 0;   // deliver inbound to the application
                addr.Loopback = 0;
                rewritten = true;
            }
        }

        if (drop)
            continue;
        (void)rewritten;
        api->send(m_networkHandle, buf.data(), len, nullptr, &addr);
    }
}
