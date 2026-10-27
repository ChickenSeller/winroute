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
// TCP only: a SOCKET-layer handle learns each outgoing connection's process (so
// per-process rules work), while a NETWORK-layer handle redirects the packets of
// the connections that need a proxy to the local relay, preserving the
// application's source port so the relay can recover the original destination
// (see FlowTable). UDP is passed through directly; fake DNS is not supported
// (host names come from sniffed real DNS responses).
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
    void socketLoop();
    void networkLoop();
    RuleEnginePtr rules() const;

    // One tracked outgoing TCP connection, keyed by the application's local port.
    enum class State { Pending, Active, Passthrough, Blocked };
    struct Conn {
        State state = State::Pending;
        quint32 pid = 0;
        QString appName;
        QString appPath;
        quint32 sessionId = 0;
        bool v6 = false;
        quint8 realDst[16] = {};   // original destination (network order), set on the first packet
        quint16 realPort = 0;
        QString hostname;          // resolved target host name, for the flow record
        RuleMatch match;           // the matched rule/action, for the flow record
    };

    // Decides and, for proxied/handled-direct connections, records the flow; sets
    // c.state. Runs on the first captured packet of the connection.
    void decide(Conn &c, const QHostAddress &dst, quint16 dport);

    InterceptorContext m_ctx;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_localDnsAvailable{true};
    quint32 m_selfPid = 0;
    quint32 m_selfSession = 0;
    quint16 m_relayPort = 0;

    HANDLE m_socketHandle = INVALID_HANDLE_VALUE;
    HANDLE m_networkHandle = INVALID_HANDLE_VALUE;
    std::thread m_socketThread;
    std::thread m_networkThread;

    mutable QMutex m_rulesMutex;
    RuleEnginePtr m_rules;
    QSet<QString> m_proxyEndpoints;   // "ip:port" of proxies, for loop detection

    mutable QMutex m_connMutex;
    QHash<quint16, Conn> m_conns;     // keyed by the application's local TCP port
};
