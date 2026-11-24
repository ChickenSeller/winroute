#pragma once

#include "Profile.h"

#include <QHostAddress>
#include <memory>
#include <vector>

struct ConnectionRequest {
    QString appPath;      // full image path, may be empty for system processes
    QString appName;      // file name, e.g. "chrome.exe"
    quint32 pid = 0;
    QHostAddress ip;      // destination address (real, not fake)
    QString hostname;     // destination host name if known
    quint16 port = 0;
};

struct RuleMatch {
    int ruleIndex = -1;
    QString ruleName;
    RuleAction action;
    DnsResolve resolve = DnsResolve::Inherit;
    int resolveProxyId = 0;
    UdpControl udp = UdpControl::Inherit;
};

// Who resolves the host name of a connection to a fake-DNS address.
struct NameResolver {
    enum Kind {
        Exit,    // the connection's own proxy gets the name (Direct: this computer)
        Local,   // this computer; a proxy then gets the address
        Proxy,   // proxyId (a proxy or chain) is asked, through a DNS query
    };
    Kind kind = Exit;
    int proxyId = 0;
};

// A profile compiled for fast matching. Immutable once built, so it can be
// shared between the packet thread and the relay thread.
class RuleEngine {
public:
    explicit RuleEngine(const Profile &profile);

    RuleMatch match(const ConnectionRequest &req) const;

    // Whether any target could send this application's UDP somewhere other
    // than straight out: through a SOCKS5 proxy, into a block rule, or out of
    // a bound adapter. When it cannot, its sockets need no filtering at all.
    bool mayDivertUdp(const QString &appPath, const QString &appName) const;

    // Whether a DNS query for this (lower-case) name gets a fake answer, so it
    // can be resolved by the proxy. The querying application is unknown
    // (Windows' DNS Client service asks for everyone). With the profile's
    // setting on (globalViaProxy), every name outside its exclusion list does;
    // otherwise names that a reachable proxy or chain rule set to resolve
    // through the proxy may match. Rules set to resolve locally get the real
    // address from the relay when their connection is made.
    bool resolveNameViaProxy(const QString &name, bool globalViaProxy) const;

    // Whether any enabled rule asks for its names to be resolved through the proxy.
    bool anyRuleResolvesViaProxy() const;

    // Whether any enabled rule turns UDP tunnelling on (udp == Enable). When one
    // does, all outgoing UDP must be filtered even if the global mode is bypass.
    bool anyRuleEnablesUdp() const;

    // Who resolves names for a connection that matched `m`. The rule's own
    // setting comes first; the profile's (on when globalViaProxy) is the
    // fallback, and its resolver choices apply only while it is on.
    NameResolver resolverFor(const RuleMatch &m, bool globalViaProxy) const;

    const Profile &profile() const { return m_profile; }

    // Exposed for tests and for the DNS exclusion list.
    static bool matchHostList(const QStringList &patterns, const QString &host, const QHostAddress &ip);
    static bool matchPortList(const QString &ports, quint16 port);
    static bool matchAppList(const QStringList &patterns, const QString &path, const QString &name);

private:
    bool viaProxy(const RuleMatch &r) const;   // rule set to resolve through a proxy
    struct Compiled {
        bool enabled;
        QStringList apps;
        QStringList targets;
        QString ports;
        RuleMatch result;
    };
    Profile m_profile;
    std::vector<Compiled> m_rules;
};

using RuleEnginePtr = std::shared_ptr<const RuleEngine>;
