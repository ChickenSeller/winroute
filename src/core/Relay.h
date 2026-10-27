#pragma once

#include "FlowTable.h"
#include "ProxyConnector.h"
#include "RuleEngine.h"

#include <QHash>
#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QTimer>

#include <functional>

class QTcpServer;
class DnsClient;
class RelaySession;
class UdpRelaySession;

struct ConnectionEvent {
    quint64 id = 0;
    quint32 pid = 0;
    QString appName;
    QString appPath;
    QString target;       // "host:port"
    QString ruleName;
    QString actionText;   // translated: "Direct", "1.2.3.4:1080 SOCKS5", "Chain Office"
    ActionType actionType = ActionType::Direct;
    bool udp = false;     // a UDP flow, not a TCP connection
    bool v6 = false;      // IPv6 (else IPv4)
    qint64 startMs = 0;
};

// One intercepted UDP datagram handed from the interceptor (NF thread) to the
// relay thread, to be tunnelled through a SOCKS5 proxy (UDP ASSOCIATE).
struct UdpDatagram {
    quint64 id = 0;        // driver endpoint id (one UDP socket)
    int proxyId = 0;       // SOCKS5 proxy the matched rule selected
    quint32 pid = 0;
    QString appName;
    QString appPath;
    QString ruleName;
    QByteArray remote;     // the address the application sent to, as sockaddr bytes
    QString hostname;      // target host name, used when the address is a fake DNS IP
    bool fakeIp = false;
    QByteArray data;       // the datagram payload
    QByteArray opts;       // NF_UDP_OPTIONS bytes (flags + options), reused for the reply
    QByteArray localEndpoint;  // app's local sockaddr (WinDivert backend injects replies to it)
    bool v6 = false;       // IPv6 target (else IPv4)
};

// Lets a backend inject UDP itself instead of going through the NetFilter driver
// API (the WinDivert backend crafts and reinjects packets). When set on the
// relay, UDP sessions deliver replies and bypassed datagrams through these.
struct UdpInjector {
    // Deliver a reply to the application: source = the address it sent to.
    std::function<void(const QByteArray &localEndpoint, const QByteArray &appRemote, const QByteArray &payload)> deliver;
    // Send a datagram out directly (proxy could not associate).
    std::function<void(const UdpDatagram &dg)> bypass;
};

struct TrafficSample {
    quint64 id = 0;
    qint64 sent = 0;
    qint64 received = 0;
};

Q_DECLARE_METATYPE(ConnectionEvent)
Q_DECLARE_METATYPE(TrafficSample)
Q_DECLARE_METATYPE(UdpDatagram)

// Local TCP relay. Lives in its own thread; accepts connections redirected by
// the interceptor and tunnels them to their targets through the configured
// proxies or directly.
class Relay : public QObject {
    Q_OBJECT
public:
    explicit Relay(FlowTable *flows, QObject *parent = nullptr);
    ~Relay() override;

    // Must be called from the relay thread.
    bool listen(QString *error);
    quint16 port() const { return m_port; }
    bool hasIPv6() const { return m_hasV6; }

    void setRules(RuleEnginePtr rules);   // thread-safe
    RuleEnginePtr rules() const;

    // Thread-safe; executed in the relay thread.
    void closeConnection(quint64 id);
    void closeAll();

    // UDP over SOCKS5. The interceptor posts datagrams here (queued); the relay
    // keeps one SOCKS5 UDP ASSOCIATE session per UDP socket. Both run in the
    // relay thread.
    void udpOut(const UdpDatagram &dg);
    void closeUdp(quint64 id);
    void udpSessionFinished(quint64 id, qint64 sent, qint64 received);

    // Set by a backend that injects UDP itself (WinDivert); empty for NetFilter.
    void setUdpInjector(UdpInjector injector) { m_udpInjector = std::move(injector); }

    DnsClient *dns() const { return m_dns; }

    // Redundancy chains remember failed proxies until the recheck time.
    bool proxyFailed(int proxyId) const;
    void markProxyFailed(int proxyId, int recheckSec);
    void resetProxyFailures();

signals:
    void opened(const ConnectionEvent &ev);
    // status: "Open", or "Error : <translated message>"; display text is up to the UI.
    void statusChanged(quint64 id, const QString &status);
    void closed(quint64 id, qint64 sent, qint64 received);
    void traffic(const QList<TrafficSample> &samples);

private:
    friend class RelaySession;
    friend class UdpRelaySession;
    void onNewConnection(QTcpServer *server);
    void sessionFinished(quint64 id);
    void emitTraffic();

    FlowTable *m_flows;
    QTcpServer *m_server4 = nullptr;
    QTcpServer *m_server6 = nullptr;
    DnsClient *m_dns = nullptr;
    quint16 m_port = 0;
    bool m_hasV6 = false;
    quint64 m_nextId = 1;
    QHash<quint64, RelaySession *> m_sessions;
    QHash<quint64, UdpRelaySession *> m_udpSessions;   // keyed by the driver endpoint id
    UdpInjector m_udpInjector;                         // set by the WinDivert backend
    QTimer m_trafficTimer;
    QTimer m_expireTimer;

    mutable QMutex m_mutex;
    RuleEnginePtr m_rules;
    QHash<int, qint64> m_failedUntil;
};
