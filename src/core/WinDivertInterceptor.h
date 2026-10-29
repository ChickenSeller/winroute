#pragma once

#include "CodeSignature.h"
#include "Interceptor.h"

#include <QHash>
#include <QHostAddress>
#include <QMutex>
#include <QSet>
#include <QString>

#include <atomic>
#include <thread>

#include <windows.h>

// Interceptor backed by WinDivert (the alternative to the NetFilter driver).
// A NETWORK-layer handle classifies each outbound TCP connection on its SYN
// (taking the owning process from the TCP table) and redirects the packets of
// the connections that need a proxy to the local relay, preserving the
// application's source port so the relay can recover the original destination
// (see FlowTable). A separate handle observes DNS: it learns IP -> host name
// from responses and, when "Resolve hostnames through proxy" is on, fake-
// resolves queries from the 198.18.0.0/15 pool just like the NetFilter backend.
// UDP is tunnelled through SOCKS5 when the profile asks for it.
class WinDivertInterceptor : public Interceptor {
public:
    WinDivertInterceptor();
    ~WinDivertInterceptor() override;

    QString name() const override { return "WinDivert"; }
    bool start(const InterceptorContext &ctx, QString *error) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    void setRules(RuleEnginePtr rules) override;
    void setLocalDnsAvailable(bool available) override { m_localDnsAvailable = available; }

    // The DLL and driver ship next to the executable; the backend is usable only
    // when they are present and loadable.
    static bool available();

    // Status for the "Network Driver" dialog and the status bar.
    struct Info {
        bool available = false;   // WinDivert.dll + WinDivert64.sys present
        bool running = false;     // the WinDivert kernel service is running
        QString version;          // WinDivert64.sys file version
        QList<codesign::Cert> certs;
    };
    static Info info();
    static bool serviceRunning();          // the "WinDivert" service is running
    static bool uninstall(QString *error); // stop and delete the WinDivert service

private:
    void networkLoop();
    void dnsLoop();          // learns IP -> host names, and fake-resolves queries
    void udpLoop();          // captures outbound UDP and tunnels proxied flows
    RuleEnginePtr rules() const;

    // "Resolve hostnames through proxy" state, mirroring the NetFilter backend.
    bool resolveViaProxy(const ProfileOptions &opt) const;
    bool shouldFakeResolve(const RuleEngine &engine, const QString &name) const;

    // UDP classification result for one (socket, target).
    enum UdpDecision { UdpBypass, UdpProxy, UdpDrop };
    int decideUdp(const RuleMatch &match, bool skip, const Profile &profile, const ProfileOptions &opt,
                  const QHostAddress &dst, quint16 dport, const QString &appName, const QString &target,
                  int *proxyId) const;

    // Injection callbacks given to the relay (run on the relay thread).
    void injectUdpReply(const QByteArray &localEndpoint, const QByteArray &appRemote, const QByteArray &payload);
    void injectUdpBypass(const struct UdpDatagram &dg);

    // One tracked outgoing TCP connection, keyed by the application's local port.
    // Classified once, on its SYN; later packets are matched to the record.
    enum class State { Active, Passthrough, Blocked };
    struct Conn {
        State state = State::Passthrough;
        quint32 pid = 0;
        QString appName;
        QString appPath;
        quint32 sessionId = 0;
        bool v6 = false;
        quint8 realDst[16] = {};   // original destination (network order), set on the first packet
        quint16 realPort = 0;
        qint64 createdMs = 0;      // for bounding the table if CLOSE events are missed
        QString hostname;          // resolved target host name, for the flow record
        bool fakeIp = false;       // destination was a fake-DNS address (host name only)
        RuleMatch match;           // the matched rule/action, for the flow record
    };

    // Decides proxy/direct/block for a connection (sets c.state), using c.pid and
    // the process fields already filled in. Runs once, on the connection's SYN.
    void decide(Conn &c, const QHostAddress &dst, quint16 dport);

    InterceptorContext m_ctx;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_localDnsAvailable{true};
    quint32 m_selfPid = 0;
    quint32 m_selfSession = 0;
    quint16 m_relayPort = 0;

    HANDLE m_networkHandle = INVALID_HANDLE_VALUE;
    HANDLE m_dnsHandle = INVALID_HANDLE_VALUE;
    HANDLE m_udpHandle = INVALID_HANDLE_VALUE;
    std::thread m_networkThread;
    std::thread m_dnsThread;
    std::thread m_udpThread;

    mutable QMutex m_rulesMutex;
    RuleEnginePtr m_rules;
    QSet<QString> m_proxyEndpoints;   // "ip:port" of proxies, for loop detection
    QSet<QString> m_proxyHosts;       // proxy host names, never fake-resolved

    mutable QMutex m_connMutex;
    QHash<quint16, Conn> m_conns;     // keyed by the application's local TCP port

    // One tracked UDP socket, keyed by the application's local UDP port. The
    // per-target decision is cached; ifIdx/v6 are kept to inject replies.
    struct UdpConn {
        quint64 id = 0;
        quint32 pid = 0;
        QString appName;
        QString appPath;
        bool skip = false;             // services / other users, when not handled
        bool v6 = false;
        quint32 ifIdx = 0;
        quint32 subIfIdx = 0;
        QHash<QString, QPair<int, int>> decisions;   // "ip:port" -> (UdpDecision, proxyId)
        qint64 lastMs = 0;
    };
    bool m_udpProxyMode = false;       // udpMode == mode_proxy at start (guarded by m_rulesMutex)
    mutable QMutex m_udpMutex;
    QHash<quint16, UdpConn> m_udpConns;
    quint64 m_nextUdpId = 1;
};
