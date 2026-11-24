#include "RuleEngine.h"
#include "NetInterfaces.h"
#include "Utils.h"

#include <QFileInfo>

RuleEngine::RuleEngine(const Profile &profile)
    : m_profile(profile)
{
    for (int i = 0; i < profile.rules.size(); ++i) {
        const Rule &r = profile.rules[i];
        Compiled c;
        c.enabled = r.enabled || i == profile.rules.size() - 1;   // Default rule is always on
        for (const QString &a : util::splitList(r.applications))
            c.apps << util::expandEnv(a);
        c.targets = util::splitList(r.targets);
        c.ports = r.ports;
        c.result.ruleIndex = i;
        c.result.ruleName = r.name;
        c.result.action = r.action;
        c.result.resolve = r.resolve;
        c.result.resolveProxyId = r.resolveProxyId;
        c.result.udp = r.udp;
        m_rules.push_back(c);
    }
}

RuleMatch RuleEngine::match(const ConnectionRequest &req) const
{
    for (const auto &c : m_rules) {
        if (!c.enabled)
            continue;
        if (!c.apps.isEmpty() && !matchAppList(c.apps, req.appPath, req.appName))
            continue;
        if (!c.targets.isEmpty() && !matchHostList(c.targets, req.hostname, req.ip))
            continue;
        if (!c.ports.trimmed().isEmpty() && !matchPortList(c.ports, req.port))
            continue;
        // A rule bound to an adapter that is down can be set to step aside.
        const RuleAction &a = c.result.action;
        if (a.skipIfInterfaceDown && !a.interfaceId.isEmpty() && !netif::isUp(a.interfaceId))
            continue;
        return c.result;
    }
    RuleMatch none;
    none.ruleName = "Default";
    return none;
}

bool RuleEngine::mayDivertUdp(const QString &appPath, const QString &appName) const
{
    for (const auto &c : m_rules) {
        if (!c.enabled)
            continue;
        if (!c.apps.isEmpty() && !matchAppList(c.apps, appPath, appName))
            continue;
        const RuleAction &a = c.result.action;
        switch (a.type) {
        case ActionType::Block:
            return true;
        case ActionType::Direct:
            if (!a.interfaceId.isEmpty())
                return true;
            break;
        case ActionType::Proxy: {
            const ProxyServer *p = m_profile.proxyById(a.id);
            // Only a SOCKS5 rule that actually tunnels UDP (enabled, or inheriting
            // while the global mode is proxy) diverts it; a disabled rule does not.
            if (p && p->type == ProxyType::Socks5
                && udpTunnels(c.result.udp, m_profile.options.udpMode == QLatin1String("mode_proxy")))
                return true;
            break;
        }
        case ActionType::Chain:
            break;   // chains carry no UDP
        }
        // A rule for every target ends the search; later rules are unreachable.
        if (c.targets.isEmpty() && c.ports.trimmed().isEmpty())
            return false;
    }
    return false;
}

bool RuleEngine::resolveNameViaProxy(const QString &name, bool globalViaProxy) const
{
    if (name.isEmpty())
        return false;
    // The profile's setting is the fallback, as before rules had their own:
    // every name outside its list. A rule resolving on this computer still
    // gets the real address, from the relay, when its connection is made.
    const ProfileOptions &opt = m_profile.options;
    if (globalViaProxy) {
        const bool listed = matchHostList(util::splitList(opt.dnsExclusionList), name, QHostAddress());
        if (opt.dnsOnlyFromList ? listed : !listed)
            return true;
    }
    // Otherwise only names a rule resolving through the proxy may match.
    for (const auto &c : m_rules) {
        if (!c.enabled)
            continue;
        if (!c.targets.isEmpty() && !matchHostList(c.targets, name, QHostAddress()))
            continue;
        if (viaProxy(c.result))
            return true;
        // A rule for every application and port ends the search.
        if (c.apps.isEmpty() && c.ports.trimmed().isEmpty())
            return false;
    }
    return false;
}

bool RuleEngine::anyRuleResolvesViaProxy() const
{
    for (const auto &c : m_rules) {
        if (c.enabled && viaProxy(c.result))
            return true;
    }
    return false;
}

bool RuleEngine::anyRuleEnablesUdp() const
{
    for (const auto &c : m_rules) {
        if (c.enabled && c.result.udp == UdpControl::Enable)
            return true;
    }
    return false;
}

// A rule whose names a proxy resolves on its own account: its own proxy or
// chain, or a chosen one. A Direct rule following its exit only uses the
// profile's choice, which applies while the profile resolves through a proxy.
bool RuleEngine::viaProxy(const RuleMatch &r) const
{
    if (r.resolve != DnsResolve::Proxy)
        return false;
    switch (r.action.type) {
    case ActionType::Proxy:
    case ActionType::Chain:
        return true;
    case ActionType::Direct:
        return r.resolveProxyId != 0 || (!m_profile.options.dnsAutoDetect && m_profile.options.dnsViaProxy
                                         && m_profile.defaultDnsId());
    case ActionType::Block:
        return false;
    }
    return false;
}

NameResolver RuleEngine::resolverFor(const RuleMatch &m, bool globalViaProxy) const
{
    const ProfileOptions &opt = m_profile.options;
    const bool direct = m.action.type == ActionType::Direct || m.action.type == ActionType::Block;
    DnsResolve mode = m.resolve;
    int id = m.resolveProxyId;
    if (mode == DnsResolve::Inherit) {
        // The profile's setting: through the rule's own proxy (Direct rules:
        // the profile's choice for them, below).
        mode = globalViaProxy ? DnsResolve::Proxy : DnsResolve::Local;
        id = 0;
    }
    NameResolver r;
    if (mode == DnsResolve::Local) {
        r.kind = NameResolver::Local;
        return r;
    }
    // A resolver that no longer exists falls back to the exit.
    if (id && !m_profile.proxyById(id) && !m_profile.chainById(id))
        id = 0;
    if (!id && direct) {
        // The default DNS proxy, only while the profile is set to resolve
        // through a proxy by hand.
        id = globalViaProxy && !opt.dnsAutoDetect ? m_profile.defaultDnsId() : 0;
        if (!id) {
            r.kind = NameResolver::Local;
            return r;
        }
    }
    if (id) {
        r.kind = NameResolver::Proxy;
        r.proxyId = id;
    }
    return r;
}

bool RuleEngine::matchAppList(const QStringList &patterns, const QString &path, const QString &name)
{
    const QString nativePath = QString(path).replace(u'/', u'\\');
    for (const QString &pat0 : patterns) {
        const QString pat = QString(pat0).replace(u'/', u'\\');
        if (pat.contains(u'\\')) {
            if (util::wildcardMatch(pat, nativePath))
                return true;
            // "C:\Program Files\App\*" style folder pattern
            if (pat.endsWith(u'\\') && nativePath.startsWith(pat, Qt::CaseInsensitive))
                return true;
        } else if (util::wildcardMatch(pat, name)) {
            return true;
        }
    }
    return false;
}

static bool parseRange(const QString &s, QHostAddress *lo, QHostAddress *hi)
{
    const qsizetype dash = s.indexOf(u'-');
    if (dash <= 0)
        return false;
    QHostAddress a(s.left(dash).trimmed()), b(s.mid(dash + 1).trimmed());
    if (a.isNull() || b.isNull() || a.protocol() != b.protocol())
        return false;
    *lo = a;
    *hi = b;
    return true;
}

static int compareAddr(const QHostAddress &a, const QHostAddress &b)
{
    if (a.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 x = a.toIPv4Address(), y = b.toIPv4Address();
        return x < y ? -1 : (x > y ? 1 : 0);
    }
    const Q_IPV6ADDR x = a.toIPv6Address(), y = b.toIPv6Address();
    return memcmp(&x, &y, sizeof(x));
}

bool RuleEngine::matchHostList(const QStringList &patterns, const QString &host, const QHostAddress &ip0)
{
    const QHostAddress ip = util::normalize(ip0);
    const QString ipStr = ip.isNull() ? QString() : ip.toString();
    for (const QString &pat : patterns) {
        if (pat.compare("%ComputerName%", Qt::CaseInsensitive) == 0) {
            if (!host.isEmpty() && host.compare(util::computerName(), Qt::CaseInsensitive) == 0)
                return true;
            continue;
        }
        if (pat.compare("%SimpleHostnames%", Qt::CaseInsensitive) == 0) {
            if (!host.isEmpty() && !host.contains(u'.'))
                return true;
            continue;
        }
        if (pat.compare("localhost", Qt::CaseInsensitive) == 0) {
            if (host.compare("localhost", Qt::CaseInsensitive) == 0 || (!ip.isNull() && ip.isLoopback()))
                return true;
            continue;
        }
        if (!ip.isNull()) {
            QHostAddress lo, hi;
            if (parseRange(pat, &lo, &hi)) {
                if (lo.protocol() == ip.protocol() && compareAddr(ip, lo) >= 0 && compareAddr(ip, hi) <= 0)
                    return true;
                continue;
            }
            if (pat.contains(u'/')) {
                const auto subnet = QHostAddress::parseSubnet(pat);
                if (!subnet.first.isNull()) {
                    if (ip.isInSubnet(subnet))
                        return true;
                    continue;
                }
            }
            const QHostAddress literal(pat);
            if (!literal.isNull()) {
                if (literal.isEqual(ip, QHostAddress::TolerantConversion))
                    return true;
                continue;
            }
            if (util::wildcardMatch(pat, ipStr))
                return true;
        }
        if (!host.isEmpty() && util::wildcardMatch(pat, host))
            return true;
    }
    return false;
}

bool RuleEngine::matchPortList(const QString &ports, quint16 port)
{
    for (const QString &item : util::splitList(ports)) {
        const qsizetype dash = item.indexOf(u'-');
        if (dash > 0) {
            const uint lo = item.left(dash).trimmed().toUInt(), hi = item.mid(dash + 1).trimmed().toUInt();
            if (port >= lo && port <= hi)
                return true;
        } else if (item.toUInt() == port) {
            return true;
        }
    }
    return false;
}
