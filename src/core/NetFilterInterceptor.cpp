#include "NetFilterInterceptor.h"
#include "DnsMessage.h"
#include "FakeDns.h"
#include "FlowTable.h"
#include "Logger.h"
#include "ProcessInfo.h"
#include "Relay.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

NetFilterInterceptor *g_instance = nullptr;

void NFAPI_CC cbThreadStart() {}
void NFAPI_CC cbThreadEnd() {}
void NFAPI_CC cbTcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO info)
{
    if (g_instance)
        g_instance->onTcpConnectRequest(id, info);
}
void NFAPI_CC cbTcpConnected(ENDPOINT_ID, PNF_TCP_CONN_INFO) {}
void NFAPI_CC cbTcpClosed(ENDPOINT_ID, PNF_TCP_CONN_INFO) {}
void NFAPI_CC cbTcpReceive(ENDPOINT_ID, const char *, int) {}
void NFAPI_CC cbTcpSend(ENDPOINT_ID, const char *, int) {}
void NFAPI_CC cbTcpCanReceive(ENDPOINT_ID) {}
void NFAPI_CC cbTcpCanSend(ENDPOINT_ID) {}
void NFAPI_CC cbUdpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO info)
{
    if (g_instance)
        g_instance->onUdpCreated(id, info);
}
void NFAPI_CC cbUdpConnectRequest(ENDPOINT_ID, PNF_UDP_CONN_REQUEST) {}
void NFAPI_CC cbUdpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO)
{
    if (g_instance)
        g_instance->onUdpClosed(id);
}
void NFAPI_CC cbUdpReceive(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt)
{
    if (g_instance)
        g_instance->onUdpReceive(id, remote, buf, len, opt);
}
void NFAPI_CC cbUdpSend(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt)
{
    if (g_instance)
        g_instance->onUdpSend(id, remote, buf, len, opt);
}
void NFAPI_CC cbUdpCanReceive(ENDPOINT_ID) {}
void NFAPI_CC cbUdpCanSend(ENDPOINT_ID) {}

NF_EventHandler g_handler = {
    cbThreadStart, cbThreadEnd,
    cbTcpConnectRequest, cbTcpConnected, cbTcpClosed, cbTcpReceive, cbTcpSend, cbTcpCanReceive, cbTcpCanSend,
    cbUdpCreated, cbUdpConnectRequest, cbUdpClosed, cbUdpReceive, cbUdpSend, cbUdpCanReceive, cbUdpCanSend,
};

QHostAddress fromSockaddr(const unsigned char *raw, quint16 *port)
{
    const auto *sa = reinterpret_cast<const sockaddr *>(raw);
    if (sa->sa_family == AF_INET) {
        const auto *in = reinterpret_cast<const sockaddr_in *>(raw);
        *port = ntohs(in->sin_port);
        return QHostAddress(ntohl(in->sin_addr.s_addr));
    }
    if (sa->sa_family == AF_INET6) {
        const auto *in6 = reinterpret_cast<const sockaddr_in6 *>(raw);
        *port = ntohs(in6->sin6_port);
        return util::normalize(QHostAddress(reinterpret_cast<const quint8 *>(&in6->sin6_addr)));
    }
    *port = 0;
    return {};
}

QString endpointKey(const QHostAddress &a, quint16 port)
{
    return util::formatEndpoint(a, port);
}

// DnsFlushResolverCache is exported by dnsapi.dll but not declared in the SDK headers.
void flushSystemDnsCache()
{
    using FlushFn = BOOL(WINAPI *)();
    HMODULE dnsapi = LoadLibraryW(L"dnsapi.dll");
    if (!dnsapi)
        return;
    auto flush = reinterpret_cast<FlushFn>(reinterpret_cast<void *>(GetProcAddress(dnsapi, "DnsFlushResolverCache")));
    if (flush && flush())
        logVerbose(QCoreApplication::translate("Interceptor", "Windows DNS cache flushed"));
    FreeLibrary(dnsapi);
}

} // namespace

NetFilterInterceptor::NetFilterInterceptor()
{
    m_selfPid = GetCurrentProcessId();
    m_selfSession = procinfo::currentSessionId();
}

NetFilterInterceptor::~NetFilterInterceptor()
{
    stop();
}

static QString driverInstallPath()
{
    wchar_t sys[MAX_PATH];
    GetSystemDirectoryW(sys, MAX_PATH);
    return QDir(QString::fromWCharArray(sys)).filePath(QString("drivers/%1.sys").arg(NetFilterInterceptor::kDriverName));
}

// Current state of the driver service (SERVICE_RUNNING etc.), 0 if it does not exist.
static DWORD serviceState()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return 0;
    DWORD state = 0;
    if (SC_HANDLE svc = OpenServiceW(scm, L"netfilter2", SERVICE_QUERY_STATUS)) {
        SERVICE_STATUS st{};
        if (QueryServiceStatus(svc, &st))
            state = st.dwCurrentState;
        CloseServiceHandle(svc);
    }
    CloseServiceHandle(scm);
    return state;
}

// nf_unRegisterDriver only disables the service, so the driver keeps running
// until reboot. Stop it and delete the service so an uninstall is complete.
static bool stopAndDeleteService(QString *error)
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        *error = QCoreApplication::translate("Interceptor", "Cannot open the service manager (error %1)").arg(GetLastError());
        return false;
    }
    SC_HANDLE svc = OpenServiceW(scm, L"netfilter2", SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (!svc) {
        const DWORD err = GetLastError();
        CloseServiceHandle(scm);
        if (err == ERROR_SERVICE_DOES_NOT_EXIST)
            return true;
        *error = QCoreApplication::translate("Interceptor", "Cannot open the driver service (error %1)").arg(err);
        return false;
    }
    SERVICE_STATUS st{};
    if (QueryServiceStatus(svc, &st) && st.dwCurrentState != SERVICE_STOPPED) {
        ControlService(svc, SERVICE_CONTROL_STOP, &st);
        for (int i = 0; i < 100 && QueryServiceStatus(svc, &st) && st.dwCurrentState != SERVICE_STOPPED; ++i)
            Sleep(100);
    }
    const bool stopped = st.dwCurrentState == SERVICE_STOPPED;
    const bool deleted = DeleteService(svc) || GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE;
    const DWORD err = GetLastError();
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    if (!deleted) {
        *error = QCoreApplication::translate("Interceptor", "Cannot delete the driver service (error %1)").arg(err);
        return false;
    }
    if (!stopped) {
        *error = QCoreApplication::translate("Interceptor", "The driver could not be stopped; it will be removed after a reboot");
        return false;
    }
    return true;
}

static bool sameFile(const QString &a, const QString &b)
{
    QFile fa(a), fb(b);
    if (!fa.open(QIODevice::ReadOnly) || !fb.open(QIODevice::ReadOnly) || fa.size() != fb.size())
        return false;
    return fa.readAll() == fb.readAll();
}

// Deletes the installed driver .sys, retrying for a few seconds. After the
// service reports STOPPED the kernel image can take a moment to finish
// unloading, during which the file stays locked; some drivers (e.g. one that
// registers many WFP callouts) are slower than others. Retrying avoids a
// spurious "reboot required" when the image just needs a little longer.
static bool deleteDriverFile(const QString &nativePath, QString *error)
{
    const auto *wpath = reinterpret_cast<LPCWSTR>(nativePath.utf16());
    for (int i = 0; i < 60; ++i) {   // up to ~15 s
        if (DeleteFileW(wpath))
            return true;
        const DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
            return true;
        if (err != ERROR_ACCESS_DENIED && err != ERROR_SHARING_VIOLATION) {
            *error = QCoreApplication::translate("Interceptor", "Cannot delete %1 (error %2)").arg(nativePath).arg(err);
            return false;
        }
        Sleep(250);
    }
    *error = QCoreApplication::translate("Interceptor", "Cannot delete %1: the driver is still loaded (close WinRoute, or restart Windows)").arg(nativePath);
    return false;
}

bool NetFilterInterceptor::installDriver(QString *error, bool replaceDifferent)
{
    const NetFilterApi *api = NetFilterApi::load(error);
    if (!api)
        return false;
    const QString installed = driverInstallPath();
    const QString bundled = QDir(QCoreApplication::applicationDirPath()).filePath(QString("%1.sys").arg(kDriverName));
    // The installer may bring a different driver than the installed one (the user
    // can choose between drivers): stop and remove the old one first.
    if (replaceDifferent && QFileInfo::exists(installed) && QFileInfo::exists(bundled) && !sameFile(installed, bundled)) {
        if (serviceState() != 0) {
            // Ask the driver to release first, so its image unloads promptly when
            // the service stops; otherwise the .sys can stay locked for a while
            // (some drivers longer than others) and the switch would seem to need
            // a reboot. uninstallDriver() does the same before stopping.
            api->unRegisterDriver(kDriverName);
            if (!stopAndDeleteService(error))
                return false;
        }
        if (!deleteDriverFile(QDir::toNativeSeparators(installed), error))
            return false;
    }
    // A disabled or stopped service (e.g. left by an earlier uninstall) is registered again.
    if (serviceState() == SERVICE_RUNNING && QFileInfo::exists(installed))
        return true;

    if (!QFileInfo::exists(installed)) {
        if (!QFileInfo::exists(bundled)) {
            *error = QCoreApplication::translate("Interceptor", "Driver file %1 was not found").arg(QDir::toNativeSeparators(bundled));
            return false;
        }
        if (!CopyFileW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(bundled).utf16()),
                       reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(installed).utf16()), FALSE)) {
            *error = QCoreApplication::translate("Interceptor", "Cannot copy the driver to %1 (error %2)").arg(QDir::toNativeSeparators(installed)).arg(GetLastError());
            return false;
        }
    }
    const NF_STATUS st = api->registerDriver(kDriverName);
    if (st != NF_STATUS_SUCCESS) {
        *error = QCoreApplication::translate("Interceptor", "Cannot register the driver: %1").arg(nfStatusString(st));
        return false;
    }
    logNormal(QCoreApplication::translate("Interceptor", "Network driver %1 installed").arg(kDriverName));
    return true;
}

bool NetFilterInterceptor::uninstallDriver(QString *error)
{
    const NetFilterApi *api = NetFilterApi::load(error);
    if (!api)
        return false;
    if (serviceState() != 0) {
        const NF_STATUS st = api->unRegisterDriver(kDriverName);
        if (st != NF_STATUS_SUCCESS) {
            *error = nfStatusString(st);
            return false;
        }
        if (!stopAndDeleteService(error))
            return false;
    }
    const QString file = QDir::toNativeSeparators(driverInstallPath());
    if (QFileInfo::exists(file) && !deleteDriverFile(file, error))
        return false;
    return true;
}

bool NetFilterInterceptor::installRules(QString *error)
{
    NF_RULE rule;
    auto add = [&](const char *what) {
        const NF_STATUS st = m_api->addRule(&rule, FALSE);
        if (st != NF_STATUS_SUCCESS && error)
            *error = QCoreApplication::translate("Interceptor", "Cannot add driver rule (%1): %2").arg(what, nfStatusString(st));
        return st == NF_STATUS_SUCCESS;
    };

    // 1. Our own traffic (relay upstream connections, resolver) is never touched.
    memset(&rule, 0, sizeof(rule));
    rule.processId = m_selfPid;
    rule.filteringFlag = NF_ALLOW;
    if (!add("self"))
        return false;

    // 2. Loopback traffic stays local.
    memset(&rule, 0, sizeof(rule));
    rule.ip_family = AF_INET;
    rule.remoteIpAddress[0] = 127;
    rule.remoteIpAddressMask[0] = 255;
    rule.filteringFlag = NF_ALLOW;
    if (!add("loopback"))
        return false;
    memset(&rule, 0, sizeof(rule));
    rule.ip_family = AF_INET6;
    rule.remoteIpAddress[15] = 1;
    memset(rule.remoteIpAddressMask, 0xFF, sizeof(rule.remoteIpAddressMask));
    rule.filteringFlag = NF_ALLOW;
    if (!add("loopback6"))
        return false;

    // 3. UDP datagrams. In proxy mode all outgoing UDP is filtered so it can be
    // tunnelled through SOCKS5; otherwise only DNS (for the fake resolver and to
    // learn IP -> host name mappings).
    bool udpProxy;
    {
        QMutexLocker lock(&m_rulesMutex);
        udpProxy = m_udpProxyMode;
    }
    memset(&rule, 0, sizeof(rule));
    rule.protocol = IPPROTO_UDP;
    rule.direction = NF_D_BOTH;
    if (!udpProxy)
        rule.remotePort = htons(53);
    rule.filteringFlag = NF_FILTER;
    if (!add("udp"))
        return false;

    // 4. Outgoing TCP connect requests are indicated to tcpConnectRequest().
    memset(&rule, 0, sizeof(rule));
    rule.protocol = IPPROTO_TCP;
    rule.direction = NF_D_OUT;
    rule.filteringFlag = NF_INDICATE_CONNECT_REQUESTS;
    return add("tcp");
}

bool NetFilterInterceptor::start(const InterceptorContext &ctx, QString *error)
{
    if (m_running)
        return true;
    m_api = NetFilterApi::load(error);
    if (!m_api)
        return false;
    if (!procinfo::isElevated()) {
        *error = QCoreApplication::translate("Interceptor", "Administrator rights are required to control the network driver");
        return false;
    }
    if (!installDriver(error))
        return false;

    m_ctx = ctx;
    g_instance = this;
    m_api->adjustProcessPriviledges();
    m_api->setOptions(4, 0);
    const NF_STATUS st = m_api->init(kDriverName, &g_handler);
    if (st != NF_STATUS_SUCCESS) {
        g_instance = nullptr;
        *error = QCoreApplication::translate("Interceptor", "Cannot connect to the network driver %1: %2").arg(kDriverName, nfStatusString(st));
        return false;
    }
    if (!installRules(error)) {
        m_api->free();
        g_instance = nullptr;
        return false;
    }
    m_running = true;
    // Names already in the Windows DNS client cache would be answered without a
    // DNS packet, so their IP -> host name mapping would never be learned.
    flushSystemDnsCache();
    return true;
}

void NetFilterInterceptor::stop()
{
    if (!m_running)
        return;
    m_running = false;
    m_api->deleteRules();
    m_api->free();
    g_instance = nullptr;
    QMutexLocker lock(&m_udpMutex);
    m_udpSocks.clear();
}

void NetFilterInterceptor::setRules(RuleEnginePtr rules)
{
    QSet<QString> hosts, endpoints;
    bool udpProxy = false;
    if (rules) {
        udpProxy = rules->profile().options.udpMode == QLatin1String("mode_proxy");
        for (const ProxyServer &p : rules->profile().proxies) {
            const QHostAddress literal(p.address);
            if (literal.isNull())
                hosts.insert(p.address.toLower());
            else
                endpoints.insert(endpointKey(util::normalize(literal), p.port));
        }
    }
    bool modeChanged;
    {
        QMutexLocker lock(&m_rulesMutex);
        modeChanged = udpProxy != m_udpProxyMode;
        m_rules = std::move(rules);
        m_proxyHosts = hosts;
        m_proxyEndpoints = endpoints;
        m_udpProxyMode = udpProxy;
    }
    // The set of filtered UDP depends on the mode, so re-apply the driver rules.
    if (modeChanged && m_running && m_api) {
        m_api->deleteRules();
        QString error;
        if (!installRules(&error))
            logError(QCoreApplication::translate("Interceptor", "Cannot update UDP filtering: %1").arg(error));
        {
            QMutexLocker lock(&m_udpMutex);
            m_udpSocks.clear();
        }
    }
}

RuleEnginePtr NetFilterInterceptor::rules() const
{
    QMutexLocker lock(&m_rulesMutex);
    return m_rules;
}

QString NetFilterInterceptor::processPath(quint32 pid, const QString &fallback) const
{
    if (!fallback.isEmpty() || !m_api)
        return fallback;
    wchar_t buf[1024];
    if (m_api->getProcessNameFromKernel(pid, buf, DWORD(std::size(buf)))) {
        wchar_t longBuf[1024];
        if (GetLongPathNameW(buf, longBuf, DWORD(std::size(longBuf))))
            return QString::fromWCharArray(longBuf);
        return QString::fromWCharArray(buf);
    }
    return {};
}

void NetFilterInterceptor::onTcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO ci)
{
    if (!m_running)
        return;
    quint16 dport = 0, sport = 0;
    const QHostAddress dst = fromSockaddr(ci->remoteAddress, &dport);
    const QHostAddress src = fromSockaddr(ci->localAddress, &sport);
    const quint32 pid = ci->processId;
    if (pid == m_selfPid || dst.isNull() || dst.isLoopback())
        return;

    const RuleEnginePtr engine = rules();
    if (!engine)
        return;
    const ProfileOptions &opt = engine->profile().options;

    procinfo::Info pinfo = procinfo::lookup(pid);
    pinfo.path = processPath(pid, pinfo.path);
    if (!pinfo.path.isEmpty())
        pinfo.name = QFileInfo(pinfo.path).fileName();
    const bool isService = pid == 4 || pinfo.sessionId == 0;
    if (isService && !opt.processServices)
        return;
    if (!isService && pinfo.sessionId != m_selfSession && !opt.processOtherUsers)
        return;

    QString host;
    const bool fake = FakeDns::isFake(dst);
    if (fake) {
        host = m_ctx.fakeDns->hostnameFor(dst);
        if (host.isEmpty()) {
            ci->filteringFlag = NF_BLOCK;
            logError(QCoreApplication::translate("Interceptor", "%1 - %2 error : unknown fake DNS address, the name was resolved before WinRoute started")
                         .arg(pinfo.name, util::formatEndpoint(dst, dport)));
            return;
        }
    } else {
        host = m_ctx.dnsCache->lookup(dst);
    }

    ConnectionRequest req;
    req.appPath = pinfo.path;
    req.appName = pinfo.name;
    req.pid = pid;
    req.ip = fake ? QHostAddress() : dst;
    req.hostname = host;
    req.port = dport;
    const RuleMatch match = engine->match(req);
    const QString target = host.isEmpty() ? util::formatEndpoint(dst, dport) : QString("%1:%2").arg(host).arg(dport);

    switch (match.action.type) {
    case ActionType::Block:
        ci->filteringFlag = NF_BLOCK;
        logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : blocked").arg(pinfo.name, target, match.ruleName));
        return;
    case ActionType::Direct:
        if (!fake && !opt.handleDirectConnections) {
            logVerbose(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : direct connection").arg(pinfo.name, target, match.ruleName));
            return;
        }
        break;
    case ActionType::Proxy:
    case ActionType::Chain: {
        if (!fake && opt.loopDetection) {
            QMutexLocker lock(&m_rulesMutex);
            if (m_proxyEndpoints.contains(endpointKey(dst, dport))) {
                lock.unlock();
                logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 is a proxy server; connecting directly to prevent a connection loop")
                              .arg(pinfo.name, target));
                return;
            }
        }
        break;
    }
    }

    const bool v6 = reinterpret_cast<const sockaddr *>(ci->remoteAddress)->sa_family == AF_INET6;
    if (v6 && !m_ctx.relayIPv6) {
        logError(QCoreApplication::translate("Interceptor", "%1 - %2 error : IPv6 relay is not available, connecting directly").arg(pinfo.name, target));
        return;
    }

    RedirectInfo info;
    info.connId = id;
    info.pid = pid;
    info.appPath = pinfo.path;
    info.appName = pinfo.name;
    info.srcAddr = src;
    info.srcPort = sport;
    info.dstAddr = dst;
    info.dstPort = dport;
    info.hostname = host;
    info.fakeIp = fake;
    info.match = match;
    info.createdMs = QDateTime::currentMSecsSinceEpoch();
    m_ctx.flows->add(info);

    // Point the connection at the relay, on the same local address when known.
    if (!v6) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_ctx.relayPort);
        const auto *local = reinterpret_cast<const sockaddr_in *>(ci->localAddress);
        addr.sin_addr.s_addr = local->sin_addr.s_addr ? local->sin_addr.s_addr : htonl(INADDR_LOOPBACK);
        memcpy(ci->remoteAddress, &addr, sizeof(addr));
    } else {
        sockaddr_in6 addr{};
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(m_ctx.relayPort);
        const auto *local = reinterpret_cast<const sockaddr_in6 *>(ci->localAddress);
        static const unsigned char zero[16] = {};
        if (memcmp(&local->sin6_addr, zero, 16) != 0)
            addr.sin6_addr = local->sin6_addr;
        else
            addr.sin6_addr.s6_addr[15] = 1;
        memcpy(ci->remoteAddress, &addr, sizeof(addr));
    }
    // Lets the driver accept a connection redirected to a local proxy.
    ci->processId = m_selfPid;
}

bool NetFilterInterceptor::resolveViaProxy(const ProfileOptions &opt) const
{
    return opt.dnsAutoDetect ? !m_localDnsAvailable : opt.dnsViaProxy;
}

bool NetFilterInterceptor::shouldFakeResolve(const RuleEngine &engine, const QString &name) const
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

void NetFilterInterceptor::onUdpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO info)
{
    if (!info)
        return;
    QMutexLocker lock(&m_udpMutex);
    UdpSock &sock = m_udpSocks[id];
    sock.pid = info->processId;
}

void NetFilterInterceptor::onUdpClosed(ENDPOINT_ID id)
{
    bool had;
    {
        QMutexLocker lock(&m_udpMutex);
        had = m_udpSocks.remove(id) > 0;
    }
    if (had && m_ctx.relay)
        m_ctx.relay->closeUdp(id);
}

void NetFilterInterceptor::onUdpSend(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len,
                                     PNF_UDP_OPTIONS opt)
{
    if (!m_running) {
        m_api->udpPostSend(id, remote, buf, len, opt);
        return;
    }
    quint16 dport = 0;
    const QHostAddress dst = fromSockaddr(remote, &dport);
    const RuleEnginePtr engine = rules();

    // DNS (port 53): fake-resolve queries and let everything else pass, as before.
    if (dport == 53) {
        if (engine && resolveViaProxy(engine->profile().options)) {
            const QByteArray query(buf, len);
            dns::Question q;
            if (dns::parseQuestion(query, &q) && !(q.flags & 0x8000) && q.qclass == 1 && shouldFakeResolve(*engine, q.name)) {
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
                    m_api->udpPostReceive(id, remote, response.constData(), int(response.size()), opt);
                    return;
                }
            }
        }
        m_api->udpPostSend(id, remote, buf, len, opt);
        return;
    }

    // Other UDP: tunnel through SOCKS5 when the mode is on, otherwise pass through.
    bool proxyMode;
    {
        QMutexLocker lock(&m_rulesMutex);
        proxyMode = m_udpProxyMode;
    }
    if (!engine || !proxyMode || dst.isNull() || dst.isLoopback()) {
        m_api->udpPostSend(id, remote, buf, len, opt);
        return;
    }
    handleUdpProxy(id, remote, buf, len, opt, engine, dst, dport);
}

void NetFilterInterceptor::handleUdpProxy(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len,
                                          PNF_UDP_OPTIONS opt, const RuleEnginePtr &engine, const QHostAddress &dst,
                                          quint16 dport)
{
    const ProfileOptions &optns = engine->profile().options;
    const bool fake = FakeDns::isFake(dst);
    const QString host = fake ? m_ctx.fakeDns->hostnameFor(dst) : m_ctx.dnsCache->lookup(dst);
    const QString remoteKey = fake ? QStringLiteral("fake:") + dst.toString() : endpointKey(dst, dport);

    // Resolve the per-socket process info and the per-target decision, caching both.
    quint32 pid = 0;
    QString appName, appPath;
    bool haveProc = false;
    int decision = UdpBypass, proxyId = 0;
    bool decided = false;
    {
        QMutexLocker lock(&m_udpMutex);
        UdpSock &sock = m_udpSocks[id];
        if (sock.pid == 0) {
            NF_UDP_CONN_INFO ci{};
            if (m_api->getUDPConnInfo && m_api->getUDPConnInfo(id, &ci) == NF_STATUS_SUCCESS)
                sock.pid = ci.processId;
        }
        pid = sock.pid;
        haveProc = sock.haveProc;
        appName = sock.appName;
        appPath = sock.appPath;
        const auto it = sock.decisions.constFind(remoteKey);
        if (it != sock.decisions.constEnd()) {
            decision = it->first;
            proxyId = it->second;
            decided = true;
        }
    }

    if (pid == m_selfPid) {
        m_api->udpPostSend(id, remote, buf, len, opt);
        return;
    }

    if (!decided) {
        bool skip = false;
        if (!haveProc) {
            procinfo::Info pinfo = procinfo::lookup(pid);
            pinfo.path = processPath(pid, pinfo.path);
            if (!pinfo.path.isEmpty())
                pinfo.name = QFileInfo(pinfo.path).fileName();
            appName = pinfo.name;
            appPath = pinfo.path;
            const bool isService = pid == 4 || pinfo.sessionId == 0;
            const bool otherUser = !isService && pinfo.sessionId != m_selfSession;
            skip = (isService && !optns.processServices) || (otherUser && !optns.processOtherUsers);
            QMutexLocker lock(&m_udpMutex);
            UdpSock &sock = m_udpSocks[id];
            sock.pid = pid;
            sock.appName = appName;
            sock.appPath = appPath;
            sock.haveProc = true;
        }
        RuleMatch match;
        if (!skip) {
            ConnectionRequest req;
            req.appPath = appPath;
            req.appName = appName;
            req.pid = pid;
            req.ip = fake ? QHostAddress() : dst;
            req.hostname = host;
            req.port = dport;
            match = engine->match(req);
        }
        const QString target = host.isEmpty() ? util::formatEndpoint(dst, dport) : QString("%1:%2").arg(host).arg(dport);
        decision = decideUdp(match, skip, engine->profile(), optns, dst, dport, fake, appName, target, &proxyId);
        QMutexLocker lock(&m_udpMutex);
        m_udpSocks[id].decisions.insert(remoteKey, {decision, proxyId});
    }

    switch (decision) {
    case UdpDrop:
        return;
    case UdpProxy: {
        if (!m_ctx.relay)
            break;
        UdpDatagram dg;
        dg.id = id;
        dg.proxyId = proxyId;
        dg.pid = pid;
        dg.appName = appName;
        dg.appPath = appPath;
        // The rule name is not cached; re-match cheaply only for display is avoided,
        // so leave it empty (the relay logs the proxy, not the rule, for UDP).
        const auto *sa = reinterpret_cast<const sockaddr *>(remote);
        int salen = sizeof(sockaddr_in);
        if (sa->sa_family == AF_INET6)
            salen = sizeof(sockaddr_in6);
        dg.remote = QByteArray(reinterpret_cast<const char *>(remote), salen);
        dg.hostname = host;
        dg.fakeIp = fake;
        dg.data = QByteArray(buf, len);
        if (opt)
            dg.opts = QByteArray(reinterpret_cast<const char *>(opt), int(sizeof(NF_UDP_OPTIONS) - 1 + qMax<long>(0, opt->optionsLength)));
        Relay *relay = m_ctx.relay;
        QMetaObject::invokeMethod(relay, [relay, dg] { relay->udpOut(dg); }, Qt::QueuedConnection);
        return;
    }
    case UdpBypass:
    default:
        break;
    }
    m_api->udpPostSend(id, remote, buf, len, opt);
}

// Decides what to do with a UDP flow (one target of one socket). Logs the
// reason once, since the result is cached per (socket, target).
int NetFilterInterceptor::decideUdp(const RuleMatch &match, bool skip, const Profile &profile, const ProfileOptions &opt,
                                    const QHostAddress &dst, quint16 dport, bool fake, const QString &appName,
                                    const QString &target, int *proxyId) const
{
    *proxyId = 0;
    if (skip)
        return UdpBypass;   // services / other users, when not handled
    switch (match.action.type) {
    case ActionType::Block:
        logNormal(QCoreApplication::translate("Interceptor", "%1 - %2 matching %3 rule : blocked (UDP)").arg(appName, target, match.ruleName));
        return UdpDrop;
    case ActionType::Direct:
        return UdpBypass;
    case ActionType::Proxy: {
        if (!fake && opt.loopDetection) {
            QMutexLocker lock(&m_rulesMutex);
            if (m_proxyEndpoints.contains(endpointKey(dst, dport)))
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

void NetFilterInterceptor::onUdpReceive(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len,
                                        PNF_UDP_OPTIONS opt)
{
    dns::Question q;
    QList<dns::Answer> answers;
    if (dns::parseAnswers(QByteArray::fromRawData(buf, len), &q, &answers)) {
        for (const auto &a : answers) {
            if ((a.type == dns::TypeA || a.type == dns::TypeAAAA) && !a.address.isNull() && !FakeDns::isFake(a.address))
                m_ctx.dnsCache->insert(a.address, q.name);
        }
    }
    m_api->udpPostReceive(id, remote, buf, len, opt);
}
