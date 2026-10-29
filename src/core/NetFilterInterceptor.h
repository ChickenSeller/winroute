#pragma once

#include "Interceptor.h"
#include "NetFilterApi.h"

#include <QHash>
#include <QMutex>
#include <QPair>
#include <QSet>
#include <QString>
#include <atomic>

// Interceptor backed by the NetFilter SDK WFP driver (netfilter2.sys).
// Outgoing TCP connect requests are indicated to tcpConnectRequest(), where
// the remote address is rewritten to the local relay for proxied connections.
// UDP port 53 traffic is filtered for the fake DNS responder and to learn
// IP -> host name mappings from DNS responses.
class NetFilterInterceptor : public Interceptor {
public:
    static constexpr const char *kDriverName = "netfilter2";

    NetFilterInterceptor();
    ~NetFilterInterceptor() override;

    QString name() const override { return "NetFilter SDK WFP driver"; }
    bool start(const InterceptorContext &ctx, QString *error) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    void setRules(RuleEnginePtr rules) override;
    void setLocalDnsAvailable(bool available) override { m_localDnsAvailable = available; }

    // Copies netfilter2.sys next to the executable into System32\drivers and registers
    // the service if needed; removes both again. Used by start() and by the installer
    // through the --install-driver / --uninstall-driver command line switches.
    // With replaceDifferent an installed driver that differs from the bundled one is replaced.
    // source overrides which .sys to install (default: <app>/netfilter2.sys).
    static bool installDriver(QString *error, bool replaceDifferent = false, const QString &source = {});
    static bool uninstallDriver(QString *error);

    // Driver management (Tools > Network Driver). The installer ships every
    // driver under <app>/drivers/<key>/netfilter2.sys so the user can switch.
    struct DriverChoice {
        QString key;    // "demo", "a", "b", "c"
        QString name;   // display name
        QString path;   // <app>/drivers/<key>/netfilter2.sys
    };
    static QList<DriverChoice> availableDrivers();
    static QString driverDisplayName(const QString &key);
    static QString installedDriverKey();   // which shipped driver the installed file matches, or {}
    static bool driverInstalled();         // the netfilter2 service exists and the file is present
    static bool driverRunning();           // the service is running

    // One Authenticode signature of the driver file.
    struct DriverCert {
        QString name;       // signer common name, for the tab title
        QString subject;    // full signer subject (multi-line)
        QString issuer;     // issuer common name
    };

    // Details of the installed driver file, for the status bar.
    struct DriverInfo {
        bool installed = false;    // the .sys file is present
        bool running = false;      // the service is running
        QString key;               // "demo"/"a"/"b"/"c", or {} if it matches none we ship
        QString name;              // display name
        bool demo = false;         // the SDK demo driver (limited) vs. an unlimited one
        QString version;           // file version, e.g. "1.7.1.8"
        QList<DriverCert> certs;   // every Authenticode signature (primary + nested)
    };
    static DriverInfo installedDriverInfo();

    // NF_EventHandler callbacks, dispatched from nfapi worker threads.
    void onTcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO info);
    void onUdpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO info);
    void onUdpClosed(ENDPOINT_ID id);
    void onUdpSend(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt);
    void onUdpReceive(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt);

private:
    bool installRules(QString *error);
    RuleEnginePtr rules() const;
    bool resolveViaProxy(const ProfileOptions &opt) const;
    bool shouldFakeResolve(const RuleEngine &rules, const QString &name) const;
    QString processPath(quint32 pid, const QString &fallback) const;
    QString dnsClient(ENDPOINT_ID id) const;   // display name of a UDP socket's process, for the DNS log
    void handleUdpProxy(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt,
                        const RuleEnginePtr &engine, const QHostAddress &dst, quint16 dport);
    int decideUdp(const RuleMatch &match, bool skip, const Profile &profile, const ProfileOptions &opt,
                  const QHostAddress &dst, quint16 dport, bool fake, const QString &appName, const QString &target,
                  int *proxyId) const;

    // Per UDP socket state, for the SOCKS5 UDP tunnelling decision.
    enum UdpDecision { UdpBypass, UdpProxy, UdpDrop };
    struct UdpSock {
        quint32 pid = 0;
        bool haveProc = false;
        QString appName;
        QString appPath;
        QHash<QString, QPair<int, int>> decisions;   // remoteKey -> (UdpDecision, proxyId)
    };

    const NetFilterApi *m_api = nullptr;
    InterceptorContext m_ctx;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_localDnsAvailable{true};
    quint32 m_selfPid = 0;
    quint32 m_selfSession = 0;

    mutable QMutex m_rulesMutex;
    RuleEnginePtr m_rules;
    QSet<QString> m_proxyHosts;        // proxy host names (never faked)
    QSet<QString> m_proxyEndpoints;    // "ip:port" of proxies (loop detection)
    bool m_udpProxyMode = false;       // ProfileOptions.udpMode == "mode_proxy" (guarded by m_rulesMutex)

    mutable QMutex m_udpMutex;
    QHash<quint64, UdpSock> m_udpSocks;   // keyed by ENDPOINT_ID
};
