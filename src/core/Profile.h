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
};

struct Rule {
    bool enabled = true;
    QString name;
    QString applications;   // "chrome.exe; "C:\a b\c.exe"; fire*.exe"
    QString targets;        // "*.example.com; 10.0.0.0-10.255.255.255"
    QString ports;          // "80; 443; 8000-9000"
    RuleAction action;
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
    bool fromXml(const QByteArray &xml, QString *error = nullptr);
    QByteArray toXml() const;

    const ProxyServer *proxyById(int id) const;
    const ProxyChain *chainById(int id) const;
    int nextId() const;   // ids are shared by proxies and chains (Proxifier uses 100+)

    QString actionDisplayName(const RuleAction &a) const;

    ProfileOptions options;
    QList<ProxyServer> proxies;
    QList<ProxyChain> chains;
    QList<Rule> rules;   // the last rule is always the "Default" rule
};
