#pragma once

#include <QList>
#include <QString>
#include <QStringList>

// Data model of a profile. The on-disk format is Proxifier's .ppx XML so that
// existing Proxifier profiles can be imported and exported unchanged.

enum class ProxyType { Socks5, Socks4, Https, Http };

QString proxyTypeToString(ProxyType t);          // "SOCKS5", "SOCKS4", "HTTPS", "HTTP"
ProxyType proxyTypeFromString(const QString &s);
QString proxyTypeDisplayName(ProxyType t);       // "SOCKS Version 5", ...

struct ProxyServer {
    int id = 0;
    ProxyType type = ProxyType::Socks5;
    QString address;
    quint16 port = 1080;
    bool authEnabled = false;
    QString username;
    QString password;
    QString label;            // Advanced: custom label ("address:port" when empty)
    bool socks4a = true;      // SOCKS4: use 4A extension (remote resolving)
    bool useHostname = true;  // Advanced: send target hostname if available
    int options = 48;         // Proxifier bit field, preserved as-is

    QString displayName() const;   // label or "address:port"
};

enum class ChainType { Simple, Redundancy, LoadBalancing };

struct ChainMember {
    int proxyId = 0;
    bool enabled = true;
};

struct ProxyChain {
    int id = 0;
    QString name;
    ChainType type = ChainType::Simple;
    QList<ChainMember> members;
    // Redundancy options
    int redundancyTimeout = 5;
    bool redundancyRecheck = true;
    int redundancyRecheckSec = 60;
    bool redundancyDirectFallback = false;
    // Load balancing option
    bool lbSamePidSameProxy = false;
};

enum class ActionType { Direct, Block, Proxy, Chain };

struct RuleAction {
    ActionType type = ActionType::Direct;
    int id = 0;   // proxy or chain id
    // The network adapter matching connections leave through (netif id, a
    // "{GUID}"; empty = the system's choice): to the target for Direct, to the
    // first proxy otherwise. Proxifier's rule "Advanced" interface setting.
    QString interfaceId;
    // When that adapter is down: skip the rule (true, Proxifier's "Ignore"),
    // or connect through the system's default adapter (false).
    bool skipIfInterfaceDown = false;
};

// How a rule's host names are resolved (WinRoute extension).
enum class DnsResolve {
    Inherit,   // the profile's Name Resolution setting
    Proxy,     // through a proxy: the rule's own (resolveProxyId 0) or a chosen one
    Local,     // on this computer: real addresses, send the IP to the proxy
};

// Whether a rule's UDP is tunnelled through its proxy (WinRoute extension).
enum class UdpControl {
    Inherit,   // follow the profile's global UDP setting (options.udpMode)
    Enable,    // tunnel this rule's UDP through its SOCKS5 proxy
    Disable,   // send this rule's UDP directly, even in global proxy mode
};

// Whether a rule whose UDP control is `udp` tunnels its UDP, given whether the
// profile's global UDP setting is "mode_proxy" (globalProxy).
inline bool udpTunnels(UdpControl udp, bool globalProxy)
{
    return udp == UdpControl::Enable || (udp == UdpControl::Inherit && globalProxy);
}

struct Rule {
    bool enabled = true;
    QString name;
    QString applications;   // "chrome.exe; "C:\a b\c.exe"; fire*.exe"
    QString targets;        // "*.example.com; 10.0.0.0-10.255.255.255"
    QString ports;          // "80; 443; 8000-9000"
    RuleAction action;
    DnsResolve resolve = DnsResolve::Inherit;
    // With resolve == Proxy: the proxy or chain (id) that resolves the names,
    // or 0 for the rule's own exit (for a Direct rule: the profile's
    // dnsDirectProxyId).
    int resolveProxyId = 0;
    UdpControl udp = UdpControl::Inherit;   // WinRoute: per-rule UDP tunnelling
};

enum class PasswordEncryption { Disabled, Basic, User, Master };

struct ProfileOptions {
    // Name resolution
    bool dnsAutoDetect = true;
    bool dnsViaProxy = false;
    bool dnsTryLocalFirst = false;
    bool dnsBlockNonATypes = false;
    bool dnsOnlyFromList = false;    // false: "Do NOT resolve the following"
    QString dnsExclusionList = "%ComputerName%; localhost; *.local";
    int dnsUdpMode = 0;
    // WinRoute: when names go through a proxy, proxy and chain rules use their
    // own; Direct rules use the default DNS proxy: this id (a proxy or chain),
    // see defaultDnsId() for when it is unset or gone.
    int dnsDirectProxyId = 0;
    QString dnsProxyServer = QStringLiteral("8.8.8.8");   // DNS server asked through a proxy
    bool logDns = false;             // WinRoute: log DNS queries and replies

    PasswordEncryption encryption = PasswordEncryption::Disabled;
    bool loopDetection = true;
    bool loopDetectionResolve = true;
    QString udpMode = "mode_bypass";
    bool leakPrevention = false;
    bool processOtherUsers = false;
    bool processServices = false;
    bool handleDirectConnections = false;
    bool httpProxiesSupport = false;
};

class Profile {
public:
    Profile();

    static Profile createDefault();

    bool load(const QString &path, QString *error = nullptr);
    bool save(const QString &path, QString *error = nullptr) const;
    // Proxifier's .ppx XML, kept for importing and exporting Proxifier profiles.
    bool fromXml(const QByteArray &xml, QString *error = nullptr);
    QByteArray toXml() const;
    // WinRoute's own native format (YAML), free of the .ppx schema. Used for
    // loading and saving profiles; .ppx is only for import/export.
    bool fromYaml(const QByteArray &yaml, QString *error = nullptr);
    QByteArray toYaml() const;
    bool loadYaml(const QString &path, QString *error = nullptr);
    bool saveYaml(const QString &path, QString *error = nullptr) const;
    // Load a profile picking the format by file extension (.ppx/.xml vs
    // .yml/.yaml), falling back to sniffing the content. For import, which
    // accepts either WinRoute (YAML) or Proxifier (.ppx) profiles.
    bool loadAuto(const QString &path, QString *error = nullptr);

    const ProxyServer *proxyById(int id) const;
    const ProxyChain *chainById(int id) const;
    // The default DNS proxy (options.dnsDirectProxyId) when it still exists,
    // else the first proxy, else the first chain; 0 only without any.
    int defaultDnsId() const;
    int nextId() const;   // ids are shared by proxies and chains (Proxifier uses 100+)

    QString actionDisplayName(const RuleAction &a) const;

    ProfileOptions options;
    QList<ProxyServer> proxies;
    QList<ProxyChain> chains;
    QList<Rule> rules;   // the last rule is always the "Default" rule
};
