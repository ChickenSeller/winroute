#include "WinDivertInterceptor.h"

#include "DnsMessage.h"
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

    // Everything is done at the network layer: WinDivert's socket layer is
    // read-only (it cannot hold a connect), so it cannot be used to classify a
    // connection before its first packet without a race. Instead each outbound
    // SYN is classified in place, taking the owning process from the TCP table.
    const std::string netFilter =
        "tcp and not impostor and ((outbound and not loopback) or (loopback and (tcp.SrcPort == "
        + std::to_string(m_relayPort) + " or tcp.DstPort == " + std::to_string(m_relayPort) + ")))";

    m_networkHandle = api->open(netFilter.c_str(), WINDIVERT_LAYER_NETWORK, -100, 0);
    if (m_networkHandle == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        if (error)
            *error = QCoreApplication::translate("Interceptor",
                         "WinDivert could not open the network layer (error %1). Administrator rights are required.").arg(err);
        return false;
    }

    // Read-only sniff of DNS responses, to learn IP -> host name mappings (the
    // relay then offers the proxy the host name, which matters for proxies that
    // reject raw addresses). Non-fatal if it cannot be opened.
    m_dnsHandle = api->open("inbound and udp and udp.SrcPort == 53", WINDIVERT_LAYER_NETWORK, -101,
                            WINDIVERT_FLAG_SNIFF | WINDIVERT_FLAG_RECV_ONLY);

    {
        QMutexLocker lock(&m_connMutex);
        m_conns.clear();
    }
    m_running = true;
    m_networkThread = std::thread([this] { networkLoop(); });
    if (m_dnsHandle != INVALID_HANDLE_VALUE)
        m_dnsThread = std::thread([this] { dnsLoop(); });
    return true;
}

void WinDivertInterceptor::stop()
{
    if (!m_running.exchange(false))
        return;
    const WinDivertApi *api = WinDivertApi::instance();
    // Shut down the recv side only: new packets then pass through natively while
    // the loop drains and reinjects whatever is already queued, so no in-flight
    // packet is dropped (closing with packets queued would hiccup even direct
    // connections). The DNS handle only sniffs, so a full shutdown is fine.
    if (m_networkHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_networkHandle, WINDIVERT_SHUTDOWN_RECV);
    if (m_dnsHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_dnsHandle, WINDIVERT_SHUTDOWN_BOTH);
    if (m_networkThread.joinable())
        m_networkThread.join();
    if (m_dnsThread.joinable())
        m_dnsThread.join();
    if (m_networkHandle != INVALID_HANDLE_VALUE) {
        api->close(m_networkHandle);
        m_networkHandle = INVALID_HANDLE_VALUE;
    }
    if (m_dnsHandle != INVALID_HANDLE_VALUE) {
        api->close(m_dnsHandle);
        m_dnsHandle = INVALID_HANDLE_VALUE;
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

    // Diagnostics (visible at Debug log level).
    qint64 lastStat = QDateTime::currentMSecsSinceEpoch();
    long nCap = 0, nDnat = 0, nRev = 0, nBlock = 0, nSendFail = 0;

    // Runs until the recv side is shut down and the queue drains (stop() uses
    // SHUTDOWN_RECV): remaining packets are still received and reinjected here,
    // and new packets pass through natively, so stopping does not drop in-flight
    // traffic - direct connections keep flowing.
    while (true) {
        if (!api->recv(m_networkHandle, buf.data(), UINT(buf.size()), &len, &addr)) {
            if (!m_running)
                break;
            continue;
        }
        ++nCap;
        wdpkt::Endpoints ep;
        if (!wdpkt::readTcp(buf.data(), len, &ep)) {
            api->send(m_networkHandle, buf.data(), len, nullptr, &addr);   // not TCP we can parse; pass through
            continue;
        }

        bool rewritten = false;
        bool drop = false;

        if (addr.Outbound && !addr.Loopback) {
            // Application -> server. Look up this connection's record (keyed by the
            // application's local port); classify it on its SYN if new.
            State st = State::Passthrough;
            quint8 rdst[16] = {};
            quint16 rport = 0;
            bool have = false;
            {
                QMutexLocker lock(&m_connMutex);
                auto it = m_conns.find(ep.srcPort);
                if (it != m_conns.end()) {
                    st = it->state;
                    std::memcpy(rdst, it->realDst, 16);
                    rport = it->realPort;
                    have = true;
                }
            }
            if (!have && ep.syn) {
                // A new outbound connection: take its owning process from the TCP
                // table (the socket is already in SYN_SENT), then decide. Doing
                // this on the SYN - which WinDivert holds until we reinject - is
                // race-free, and the relay's own upstream connections resolve to
                // our pid and are left alone.
                const QHostAddress src = hostFromBytes(ep.src, ep.v6);
                const QHostAddress dst = hostFromBytes(ep.dst, ep.v6);
                Conn c;
                c.pid = procinfo::tcpOwnerPid(src, ep.srcPort);
                c.v6 = ep.v6;
                std::memcpy(c.realDst, ep.dst, 16);
                c.realPort = ep.dstPort;
                c.createdMs = QDateTime::currentMSecsSinceEpoch();
                if (c.pid == 0 || c.pid == m_selfPid) {
                    c.state = State::Passthrough;   // unknown, or our own traffic
                } else {
                    procinfo::Info pinfo = procinfo::lookup(c.pid);
                    c.appPath = pinfo.path;
                    c.appName = pinfo.path.isEmpty() ? pinfo.name : QFileInfo(pinfo.path).fileName();
                    c.sessionId = pinfo.sessionId;
                    decide(c, dst, ep.dstPort);
                    if (c.state == State::Active) {
                        RedirectInfo info;
                        info.pid = c.pid;
                        info.appPath = c.appPath;
                        info.appName = c.appName;
                        info.srcAddr = src;
                        info.srcPort = ep.srcPort;
                        info.dstAddr = dst;
                        info.dstPort = ep.dstPort;
                        info.hostname = c.hostname;
                        info.match = c.match;
                        info.createdMs = c.createdMs;
                        if (m_ctx.flows)
                            m_ctx.flows->add(info);
                    }
                }
                QMutexLocker lock(&m_connMutex);
                if (m_conns.size() > 8192) {   // bound growth if some closes are missed
                    const qint64 cutoff = c.createdMs - 120000;
                    for (auto i = m_conns.begin(); i != m_conns.end();) {
                        if (i->createdMs < cutoff)
                            i = m_conns.erase(i);
                        else
                            ++i;
                    }
                }
                st = c.state;
                std::memcpy(rdst, c.realDst, 16);
                rport = c.realPort;
                have = true;
                m_conns.insert(ep.srcPort, c);
            }
            if (have) {
                // Scope the action to this connection's real destination: the
                // source port alone is not unique (the relay's own upstream
                // connections draw from the same ephemeral range).
                const bool toRealDst = ep.dstPort == rport && std::memcmp(ep.dst, rdst, 16) == 0;
                if (st == State::Active && toRealDst) {
                    // Self-redirect: destination becomes the source IP, relay port;
                    // the source port is preserved so the relay recovers the flow.
                    wdpkt::setTcpDst(buf.data(), len, ep.src, m_relayPort);
                    addr.Outbound = 0;   // deliver inbound to the local relay
                    addr.Loopback = 1;
                    rewritten = true;
                    ++nDnat;
                } else if (st == State::Blocked && toRealDst) {
                    drop = true;
                    ++nBlock;
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
                ++nRev;
            }
        }

        if (drop)
            continue;
        (void)rewritten;
        if (!api->send(m_networkHandle, buf.data(), len, nullptr, &addr))
            ++nSendFail;

        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastStat >= 2000) {
            lastStat = now;
            int conns;
            { QMutexLocker lock(&m_connMutex); conns = m_conns.size(); }
            logDebug(QStringLiteral("WinDivert: cap=%1 dnat=%2 rev=%3 block=%4 sendfail=%5 conns=%6")
                         .arg(nCap).arg(nDnat).arg(nRev).arg(nBlock).arg(nSendFail).arg(conns));
        }
    }
}

void WinDivertInterceptor::dnsLoop()
{
    const WinDivertApi *api = WinDivertApi::instance();
    std::vector<unsigned char> buf(WINDIVERT_MTU_MAX);
    WINDIVERT_ADDRESS addr;
    UINT len = 0;
    while (m_running) {
        if (!api->recv(m_dnsHandle, buf.data(), UINT(buf.size()), &len, &addr)) {
            if (!m_running)
                break;
            continue;
        }
        const unsigned char *payload = nullptr;
        unsigned plen = 0;
        if (!wdpkt::udpPayload(buf.data(), len, &payload, &plen) || !m_ctx.dnsCache)
            continue;   // sniffing only; nothing is reinjected
        dns::Question q;
        QList<dns::Answer> answers;
        if (!dns::parseAnswers(QByteArray::fromRawData(reinterpret_cast<const char *>(payload), int(plen)), &q, &answers))
            continue;
        for (const auto &a : answers) {
            if ((a.type == dns::TypeA || a.type == dns::TypeAAAA) && !a.address.isNull() && !FakeDns::isFake(a.address))
                m_ctx.dnsCache->insert(a.address, q.name);
        }
    }
}
