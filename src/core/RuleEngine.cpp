#include "RuleEngine.h"
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
        return c.result;
    }
    RuleMatch none;
    none.ruleName = "Default";
    return none;
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
