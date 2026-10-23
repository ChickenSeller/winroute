#pragma once

#include "FlowTable.h"
#include "ProxyConnector.h"
#include "RuleEngine.h"

#include <QHash>
#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QTimer>

class QTcpServer;
class DnsClient;
class RelaySession;

struct ConnectionEvent {
    quint64 id = 0;
    quint32 pid = 0;
    QString appName;
    QString appPath;
    QString target;       // "host:port"
    QString ruleName;
    QString actionText;   // translated: "Direct", "1.2.3.4:1080 SOCKS5", "Chain Office"
    ActionType actionType = ActionType::Direct;
    qint64 startMs = 0;
};

struct TrafficSample {
    quint64 id = 0;
    qint64 sent = 0;
    qint64 received = 0;
};

Q_DECLARE_METATYPE(ConnectionEvent)
Q_DECLARE_METATYPE(TrafficSample)

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
    QTimer m_trafficTimer;
    QTimer m_expireTimer;

    mutable QMutex m_mutex;
    RuleEnginePtr m_rules;
    QHash<int, qint64> m_failedUntil;
};
