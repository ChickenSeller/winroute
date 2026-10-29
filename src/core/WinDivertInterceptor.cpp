#include <winsock2.h>
#include <ws2tcpip.h>

#include "WinDivertInterceptor.h"

#include "DnsMessage.h"
#include "FakeDns.h"      // DnsCache
#include "FlowTable.h"
#include "Logger.h"
#include "ProcessInfo.h"
#include "Relay.h"
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

// Packs network-order address bytes + port into sockaddr_in/in6 bytes (the form
// UdpDatagram.remote/localEndpoint and UdpRelaySession::targetText expect).
QByteArray makeSockaddr(const quint8 *addr, bool v6, quint16 port)
{
    if (v6) {
        sockaddr_in6 sa{};
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(port);
        std::memcpy(&sa.sin6_addr, addr, 16);
        return QByteArray(reinterpret_cast<const char *>(&sa), sizeof(sa));
    }
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    std::memcpy(&sa.sin_addr, addr, 4);
    return QByteArray(reinterpret_cast<const char *>(&sa), sizeof(sa));
}

bool parseSockaddr(const QByteArray &b, quint8 out[16], bool *v6, quint16 *port)
{
    std::memset(out, 0, 16);
    if (b.size() < int(sizeof(sockaddr)))
        return false;
    const auto *sa = reinterpret_cast<const sockaddr *>(b.constData());
    if (sa->sa_family == AF_INET6 && b.size() >= int(sizeof(sockaddr_in6))) {
        const auto *s6 = reinterpret_cast<const sockaddr_in6 *>(b.constData());
        std::memcpy(out, &s6->sin6_addr, 16);
        *port = ntohs(s6->sin6_port);
        *v6 = true;
        return true;
    }
    if (sa->sa_family == AF_INET && b.size() >= int(sizeof(sockaddr_in))) {
        const auto *s4 = reinterpret_cast<const sockaddr_in *>(b.constData());
        std::memcpy(out, &s4->sin_addr, 4);
        *port = ntohs(s4->sin_port);
        *v6 = false;
        return true;
    }
    return false;
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

    // DNS, both directions. Responses are observed to learn IP -> host name
    // mappings (the relay then offers the proxy the host name, which matters for
    // proxies that reject raw addresses); outbound queries are fake-resolved from
    // the 198.18.0.0/15 pool when "Resolve hostnames through proxy" is on, and
    // otherwise reinjected unchanged. Our own injected replies carry the impostor
    // flag so they are not captured again. Non-fatal if it cannot be opened.
    m_dnsHandle = api->open("udp and not impostor and (udp.DstPort == 53 or udp.SrcPort == 53)",
                            WINDIVERT_LAYER_NETWORK, -101, 0);

    // UDP over SOCKS5, when the profile asks for it: capture outbound UDP (except
    // DNS, which the sniff above needs to see) and tunnel the proxied flows. The
    // relay injects replies and bypassed datagrams back through us.
    bool udpProxy;
    { QMutexLocker lock(&m_rulesMutex); udpProxy = m_udpProxyMode; }
    if (udpProxy) {
        m_udpHandle = api->open("outbound and udp and not loopback and not impostor and udp.DstPort != 53",
                                WINDIVERT_LAYER_NETWORK, -102, 0);
        if (m_udpHandle != INVALID_HANDLE_VALUE && m_ctx.relay) {
            UdpInjector inj;
            inj.deliver = [this](const QByteArray &local, const QByteArray &appRemote, const QByteArray &payload) {
                injectUdpReply(local, appRemote, payload);
            };
            inj.bypass = [this](const UdpDatagram &dg) { injectUdpBypass(dg); };
            m_ctx.relay->setUdpInjector(inj);
        }
    }

    {
        QMutexLocker lock(&m_connMutex);
        m_conns.clear();
    }
    { QMutexLocker lock(&m_udpMutex); m_udpConns.clear(); }
    m_running = true;
    m_networkThread = std::thread([this] { networkLoop(); });
    if (m_dnsHandle != INVALID_HANDLE_VALUE)
        m_dnsThread = std::thread([this] { dnsLoop(); });
    if (m_udpHandle != INVALID_HANDLE_VALUE)
        m_udpThread = std::thread([this] { udpLoop(); });
    return true;
}

void WinDivertInterceptor::stop()
{
    if (!m_running.exchange(false))
        return;
    const WinDivertApi *api = WinDivertApi::instance();
    // Shut down the recv side only: new packets then pass through natively while
    // each loop drains and reinjects whatever is already queued, so no in-flight
    // packet is dropped (closing with packets queued would hiccup even direct
    // connections). The DNS handle now reinjects traffic too, so it drains the
    // same way rather than being torn down outright.
    if (m_ctx.relay)
        m_ctx.relay->setUdpInjector(UdpInjector{});
    if (m_networkHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_networkHandle, WINDIVERT_SHUTDOWN_RECV);
    if (m_udpHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_udpHandle, WINDIVERT_SHUTDOWN_RECV);
    if (m_dnsHandle != INVALID_HANDLE_VALUE)
        api->shutdown(m_dnsHandle, WINDIVERT_SHUTDOWN_RECV);
    if (m_networkThread.joinable())
        m_networkThread.join();
    if (m_udpThread.joinable())
        m_udpThread.join();
    if (m_dnsThread.joinable())
        m_dnsThread.join();
    for (HANDLE *h : {&m_networkHandle, &m_udpHandle, &m_dnsHandle}) {
        if (*h != INVALID_HANDLE_VALUE) {
            api->close(*h);
            *h = INVALID_HANDLE_VALUE;
        }
    }
    { QMutexLocker lock(&m_udpMutex); m_udpConns.clear(); }
    QMutexLocker lock(&m_connMutex);
    m_conns.clear();
}

void WinDivertInterceptor::setRules(RuleEnginePtr rules)
{
    QSet<QString> endpoints, hosts;
    bool udpProxy = false;
    if (rules) {
        udpProxy = rules->profile().options.udpMode == QLatin1String("mode_proxy");
        for (const ProxyServer &p : rules->profile().proxies) {
            const QHostAddress literal(p.address);
            if (literal.isNull())
                hosts.insert(p.address.toLower());   // a host name, never fake-resolved
            else
                endpoints.insert(util::formatEndpoint(util::normalize(literal), p.port));
        }
    }
    QMutexLocker lock(&m_rulesMutex);
    m_rules = std::move(rules);
    m_proxyEndpoints = endpoints;
    m_proxyHosts = hosts;
    m_udpProxyMode = udpProxy;   // whether to tunnel UDP; read by start()
}

RuleEnginePtr WinDivertInterceptor::rules() const
{
    QMutexLocker lock(&m_rulesMutex);
    return m_rules;
}

bool WinDivertInterceptor::resolveViaProxy(const ProfileOptions &opt) const
{
    return opt.dnsAutoDetect ? !m_localDnsAvailable : opt.dnsViaProxy;
}

bool WinDivertInterceptor::shouldFakeResolve(const RuleEngine &engine, const QString &name) const
{
    const ProfileOptions &opt = engine.profile().options;
    if (!resolveViaProxy(opt) || name.isEmpty())
        return false;
    const QString lower = name.toLower();
    if (lower.endsWith(".arpa") || lower == "localhost" || lower.endsWith(".localhost"))
        return false;
    {
        QMutexLocker lock(&m_rulesMutex);
        if (m_proxyHosts.contains(lower))
            return false;
    }
    const bool listed = RuleEngine::matchHostList(util::splitList(opt.dnsExclusionList), lower, QHostAddress());
    return opt.dnsOnlyFromList ? listed : !listed;
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

    // A fake-DNS address stands for a host name we handed the application; the
    // real target is only known by that name (there is no server at the fake
    // address), so a fake connection always goes to the relay - even on a Direct
    // rule, where the relay connects out to the name itself.
    const bool fake = m_ctx.fakeDns && FakeDns::isFake(dst);
    QString host;
    if (fake) {
        host = m_ctx.fakeDns->hostnameFor(dst);
        if (host.isEmpty()) {
            c.state = State::Blocked;
            logError(QCoreApplication::translate("Interceptor", "%1 - %2 error : unknown fake DNS address, the name was resolved before WinRoute started")
                         .arg(c.appName, util::formatEndpoint(dst, dport)));
            return;
        }
    } else {
        host = m_ctx.dnsCache ? m_ctx.dnsCache->lookup(dst) : QString();
    }

    ConnectionRequest req;
    req.appPath = c.appPath;
    req.appName = c.appName;
    req.pid = c.pid;
    req.ip = fake ? QHostAddress() : dst;
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
        if (!fake && !opt.handleDirectConnections) {
            c.state = State::Passthrough;
            logVerbose(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : direct connection").arg(c.appName, target, match.ruleName));
            return;
        }
        break;
    case ActionType::Proxy:
    case ActionType::Chain:
        if (!fake && opt.loopDetection) {
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
    c.fakeIp = fake;
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
                        info.fakeIp = c.fakeIp;
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

    // Drains on stop() the same way as networkLoop (SHUTDOWN_RECV): queued DNS
    // is still reinjected and new DNS passes through natively.
    while (true) {
        if (!api->recv(m_dnsHandle, buf.data(), UINT(buf.size()), &len, &addr)) {
            if (!m_running)
                break;
            continue;
        }
        wdpkt::Endpoints ep;
        const unsigned char *payload = nullptr;
        unsigned plen = 0;
        if (!wdpkt::readUdp(buf.data(), len, &ep, &payload, &plen)) {
            api->send(m_dnsHandle, buf.data(), len, nullptr, &addr);   // not UDP we can parse
            continue;
        }

        // Inbound response: learn IP -> host name, then let it reach the app.
        if (ep.srcPort == 53) {
            if (m_ctx.dnsCache) {
                dns::Question q;
                QList<dns::Answer> answers;
                if (dns::parseAnswers(QByteArray::fromRawData(reinterpret_cast<const char *>(payload), int(plen)), &q, &answers))
                    for (const auto &a : answers)
                        if ((a.type == dns::TypeA || a.type == dns::TypeAAAA) && !a.address.isNull() && !FakeDns::isFake(a.address))
                            m_ctx.dnsCache->insert(a.address, q.name);
            }
            api->send(m_dnsHandle, buf.data(), len, nullptr, &addr);
            continue;
        }

        // Outbound query (DstPort == 53). Fake-resolve it when the profile asks
        // to resolve names through the proxy; otherwise send it on unchanged.
        bool answered = false;
        const RuleEnginePtr engine = rules();
        if (engine && m_ctx.fakeDns && resolveViaProxy(engine->profile().options)) {
            const QByteArray query(reinterpret_cast<const char *>(payload), int(plen));
            dns::Question q;
            // Never fake-resolve WinRoute's own look-ups: the relay resolves names
            // for direct connections, and a fake answer there would misdirect it.
            const quint32 owner = procinfo::udpOwnerPid(hostFromBytes(ep.src, ep.v6), ep.srcPort);
            if (owner != m_selfPid && dns::parseQuestion(query, &q) && !(q.flags & 0x8000) && q.qclass == 1
                && shouldFakeResolve(*engine, q.name)) {
                const bool blockOthers = engine->profile().options.dnsBlockNonATypes;
                QByteArray response;
                if (q.qtype == dns::TypeA) {
                    const QHostAddress fake = m_ctx.fakeDns->addressFor(q.name);
                    response = dns::buildResponse(query, q, {fake}, 10);
                    logDebug(QCoreApplication::translate("Interceptor", "DNS %1 resolved through proxy as %2").arg(q.name, fake.toString()));
                } else if (q.qtype == dns::TypeAAAA || q.qtype == dns::TypeHTTPS || blockOthers) {
                    response = dns::buildResponse(query, q, {}, 10);
                }
                if (!response.isEmpty()) {
                    std::vector<unsigned char> pkt(size_t(ep.v6 ? 40 : 20) + 8 + response.size());
                    const unsigned n = wdpkt::buildUdp(ep.v6, ep.dst, 53, ep.src, ep.srcPort,
                                                       reinterpret_cast<const uint8_t *>(response.constData()),
                                                       unsigned(response.size()), pkt.data(), unsigned(pkt.size()));
                    if (n) {
                        WINDIVERT_ADDRESS rep = addr;
                        rep.Outbound = 0;   // inbound: delivered back to the application
                        rep.Impostor = 1;   // so our own reply is not captured again
                        api->send(m_dnsHandle, pkt.data(), n, nullptr, &rep);
                        answered = true;
                    }
                }
            }
        }
        if (!answered)
            api->send(m_dnsHandle, buf.data(), len, nullptr, &addr);
    }
}

int WinDivertInterceptor::decideUdp(const RuleMatch &match, bool skip, const Profile &profile,
                                    const ProfileOptions &opt, const QHostAddress &dst, quint16 dport,
                                    const QString &appName, const QString &target, int *proxyId) const
{
    *proxyId = 0;
    if (skip)
        return UdpBypass;
    switch (match.action.type) {
    case ActionType::Block:
        logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : blocked (UDP)").arg(appName, target, match.ruleName));
        return UdpDrop;
    case ActionType::Direct:
        return UdpBypass;
    case ActionType::Proxy: {
        if (opt.loopDetection) {
            QMutexLocker lock(&m_rulesMutex);
            if (m_proxyEndpoints.contains(util::formatEndpoint(dst, dport)))
                return UdpBypass;
        }
        const ProxyServer *p = profile.proxyById(match.action.id);
        if (!p)
            return UdpBypass;
        if (p->type == ProxyType::Socks5) {
            *proxyId = p->id;
            return UdpProxy;
        }
        logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : %4 does not support UDP, sending directly")
                      .arg(appName, target, match.ruleName, proxyTypeToString(p->type)));
        return UdpBypass;
    }
    case ActionType::Chain:
        logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : proxy chains do not support UDP, sending directly").arg(appName, target, match.ruleName));
        return UdpBypass;
    }
    return UdpBypass;
}

void WinDivertInterceptor::udpLoop()
{
    const WinDivertApi *api = WinDivertApi::instance();
    std::vector<unsigned char> buf(WINDIVERT_MTU_MAX);
    WINDIVERT_ADDRESS addr;
    UINT len = 0;

    while (true) {
        if (!api->recv(m_udpHandle, buf.data(), UINT(buf.size()), &len, &addr)) {
            if (!m_running)
                break;
            continue;
        }
        wdpkt::Endpoints ep;
        const unsigned char *payload = nullptr;
        unsigned plen = 0;
        if (!wdpkt::readUdp(buf.data(), len, &ep, &payload, &plen)) {
            api->send(m_udpHandle, buf.data(), len, nullptr, &addr);
            continue;
        }
        const QHostAddress src = hostFromBytes(ep.src, ep.v6);
        const QHostAddress dst = hostFromBytes(ep.dst, ep.v6);
        const QString targetKey = util::formatEndpoint(dst, ep.dstPort);

        quint64 sessId = 0;
        bool known = false, skip = false;
        quint32 pid = 0;
        QString appName, appPath;
        int decision = -1, proxyId = 0;
        {
            QMutexLocker lock(&m_udpMutex);
            UdpConn &c = m_udpConns[ep.srcPort];
            c.lastMs = QDateTime::currentMSecsSinceEpoch();
            known = c.id != 0;
            if (!known) {
                c.id = m_nextUdpId++;
                c.v6 = ep.v6;
                c.ifIdx = addr.Network.IfIdx;
                c.subIfIdx = addr.Network.SubIfIdx;
            } else if (!c.skip) {
                auto it = c.decisions.constFind(targetKey);
                if (it != c.decisions.constEnd()) {
                    decision = it->first;
                    proxyId = it->second;
                }
            }
            sessId = c.id;
            skip = c.skip;
            pid = c.pid;
            appName = c.appName;
            appPath = c.appPath;
        }

        if (!known) {
            // Classify the new socket by its owning process (TCP table analogue).
            const quint32 owner = procinfo::udpOwnerPid(src, ep.srcPort);
            bool s = owner == 0 || owner == m_selfPid;
            QString aName, aPath;
            if (!s) {
                procinfo::Info pinfo = procinfo::lookup(owner);
                aPath = pinfo.path;
                aName = pinfo.path.isEmpty() ? pinfo.name : QFileInfo(pinfo.path).fileName();
                const RuleEnginePtr engine = rules();
                const ProfileOptions opt = engine ? engine->profile().options : ProfileOptions{};
                const bool isService = owner == 4 || pinfo.sessionId == 0;
                const bool otherUser = !isService && pinfo.sessionId != m_selfSession;
                s = (isService && !opt.processServices) || (otherUser && !opt.processOtherUsers);
            }
            QMutexLocker lock(&m_udpMutex);
            UdpConn &c = m_udpConns[ep.srcPort];
            c.pid = owner;
            c.appName = aName;
            c.appPath = aPath;
            c.skip = s;
            skip = s;
            pid = owner;
            appName = aName;
            appPath = aPath;
        }

        // A fake-DNS address stands for a host name; recover it so the datagram is
        // tunnelled to the name (there is no server at the fake address), exactly
        // as the NetFilter backend does.
        const bool fakeDst = m_ctx.fakeDns && FakeDns::isFake(dst);

        if (!skip && decision < 0) {
            const RuleEnginePtr engine = rules();
            if (!engine) {
                decision = UdpBypass;
            } else {
                const QString host = fakeDst ? m_ctx.fakeDns->hostnameFor(dst)
                                             : (m_ctx.dnsCache ? m_ctx.dnsCache->lookup(dst) : QString());
                ConnectionRequest req;
                req.appPath = appPath;
                req.appName = appName;
                req.pid = pid;
                req.ip = fakeDst ? QHostAddress() : dst;
                req.hostname = host;
                req.port = ep.dstPort;
                const RuleMatch match = engine->match(req);
                const QString target = host.isEmpty() ? targetKey : QString("%1:%2").arg(host).arg(ep.dstPort);
                decision = decideUdp(match, false, engine->profile(), engine->profile().options, dst, ep.dstPort, appName, target, &proxyId);
                QMutexLocker lock(&m_udpMutex);
                auto it = m_udpConns.find(ep.srcPort);
                if (it != m_udpConns.end())
                    it->decisions.insert(targetKey, {decision, proxyId});
            }
        }
        if (skip)
            decision = UdpBypass;

        if (decision == UdpDrop)
            continue;   // blocked
        if (decision == UdpProxy && m_ctx.relay) {
            UdpDatagram dg;
            dg.id = sessId;
            dg.proxyId = proxyId;
            dg.pid = pid;
            dg.appName = appName;
            dg.appPath = appPath;
            dg.remote = makeSockaddr(ep.dst, ep.v6, ep.dstPort);
            dg.localEndpoint = makeSockaddr(ep.src, ep.v6, ep.srcPort);
            dg.v6 = ep.v6;
            dg.hostname = fakeDst ? m_ctx.fakeDns->hostnameFor(dst)
                                  : (m_ctx.dnsCache ? m_ctx.dnsCache->lookup(dst) : QString());
            dg.data = QByteArray(reinterpret_cast<const char *>(payload), int(plen));
            Relay *relay = m_ctx.relay;
            QMetaObject::invokeMethod(relay, [relay, dg] { relay->udpOut(dg); }, Qt::QueuedConnection);
            continue;   // consumed: the relay sends it through the proxy
        }
        // Bypass: let the datagram go out unchanged.
        api->send(m_udpHandle, buf.data(), len, nullptr, &addr);
    }
}

void WinDivertInterceptor::injectUdpReply(const QByteArray &localEndpoint, const QByteArray &appRemote,
                                          const QByteArray &payload)
{
    if (m_udpHandle == INVALID_HANDLE_VALUE)
        return;
    quint8 la[16], ra[16];
    bool lv6, rv6;
    quint16 lport, rport;
    if (!parseSockaddr(localEndpoint, la, &lv6, &lport) || !parseSockaddr(appRemote, ra, &rv6, &rport) || lv6 != rv6)
        return;
    std::vector<unsigned char> pkt(size_t(lv6 ? 40 : 20) + 8 + payload.size());
    // Reply appears to come from the address the app sent to (ra), to the app (la).
    const unsigned n = wdpkt::buildUdp(lv6, ra, rport, la, lport,
                                       reinterpret_cast<const uint8_t *>(payload.constData()), unsigned(payload.size()),
                                       pkt.data(), unsigned(pkt.size()));
    if (!n)
        return;
    WINDIVERT_ADDRESS addr{};
    addr.Outbound = 0;   // inbound: delivered to the application
    addr.IPv6 = lv6 ? 1 : 0;
    {
        QMutexLocker lock(&m_udpMutex);
        auto it = m_udpConns.find(lport);
        if (it != m_udpConns.end()) {
            addr.Network.IfIdx = it->ifIdx;
            addr.Network.SubIfIdx = it->subIfIdx;
        }
    }
    WinDivertApi::instance()->send(m_udpHandle, pkt.data(), n, nullptr, &addr);
}

void WinDivertInterceptor::injectUdpBypass(const UdpDatagram &dg)
{
    if (m_udpHandle == INVALID_HANDLE_VALUE)
        return;
    quint8 la[16], ra[16];
    bool lv6, rv6;
    quint16 lport, rport;
    if (!parseSockaddr(dg.localEndpoint, la, &lv6, &lport) || !parseSockaddr(dg.remote, ra, &rv6, &rport) || lv6 != rv6)
        return;
    std::vector<unsigned char> pkt(size_t(lv6 ? 40 : 20) + 8 + dg.data.size());
    const unsigned n = wdpkt::buildUdp(lv6, la, lport, ra, rport,
                                       reinterpret_cast<const uint8_t *>(dg.data.constData()), unsigned(dg.data.size()),
                                       pkt.data(), unsigned(pkt.size()));
    if (!n)
        return;
    WINDIVERT_ADDRESS addr{};
    addr.Outbound = 1;   // outbound: sent directly to the target
    addr.IPv6 = lv6 ? 1 : 0;
    {
        QMutexLocker lock(&m_udpMutex);
        auto it = m_udpConns.find(lport);
        if (it != m_udpConns.end()) {
            addr.Network.IfIdx = it->ifIdx;
            addr.Network.SubIfIdx = it->subIfIdx;
        }
    }
    WinDivertApi::instance()->send(m_udpHandle, pkt.data(), n, nullptr, &addr);
}
