#pragma once

#include "RuleEngine.h"

#include <QHash>
#include <QHostAddress>
#include <QMutex>
#include <QObject>

class QTcpServer;
class QTcpSocket;
class DnsClient;
class GatewaySession;

// Inbound proxy server: lets other machines reach the network through this one.
// It listens for HTTP/HTTPS(CONNECT) and SOCKS5 clients and forwards each
// connection out either directly or through this machine's own proxy rules
// (the same RuleEngine and ProxyConnector the relay uses). It does not need the
// capture driver: it works purely at the TCP level.
//
// Lives in its own thread; start()/stop() must be called there.
class ProxyGateway : public QObject {
    Q_OBJECT
public:
    // How an accepted connection is sent on.
    enum class Mode {
        Rules,   // match this machine's proxy rules (direct/proxy/chain/block)
        Direct,  // always connect straight out, ignoring the rules
        Proxy,   // send everything through one chosen proxy or chain (the "exit")
    };

    // What to do when the selected proxy or chain cannot be used: it no longer
    // exists in the profile (or has no usable member), or it cannot be connected.
    enum class OnMissing {
        Block,   // refuse the connection, so nothing silently bypasses the proxy
        Direct,  // fall back to a direct connection
    };

    struct Config {
        bool enabled = false;                             // master switch for the whole feature
        QHostAddress listenAddress = QHostAddress::Any;   // bind; Any = every interface
        // The two protocols listen on their own ports and can be enabled apart.
        bool httpEnabled = true;
        quint16 httpPort = 8080;
        bool socksEnabled = true;
        quint16 socksPort = 1080;
        Mode mode = Mode::Rules;
        OnMissing onMissing = OnMissing::Block;
        // When mode == Proxy, the fixed exit every connection is sent through:
        // a proxy or a chain of the active profile, named the same {type, id}
        // way a rule's action is. onMissing applies if it is gone.
        RuleAction exit;   // type is Proxy or Chain
        // An empty user name allows anonymous clients; otherwise clients must
        // authenticate (SOCKS5 username/password, HTTP Basic) with these.
        QString username;
        QString password;
    };

    explicit ProxyGateway(QObject *parent = nullptr);
    ~ProxyGateway() override;

    // Called in the gateway thread. Returns false (and sets error) if the port
    // cannot be bound.
    bool start(const Config &config, QString *error);
    void stop();
    bool isRunning() const { return m_httpServer || m_socksServer; }
    quint16 httpPort() const { return m_httpPort; }    // bound HTTP/HTTPS port, 0 if off
    quint16 socksPort() const { return m_socksPort; }  // bound SOCKS5 port, 0 if off

    void setRules(RuleEnginePtr rules);   // thread-safe
    RuleEnginePtr rules() const;

    Config config() const { return m_config; }
    DnsClient *dns() const { return m_dns; }

    // The configuration kept in WinRoute.ini (the "proxyServer" group).
    static Config loadConfig();
    static void saveConfig(const Config &config);

private:
    friend class GatewaySession;
    enum class Proto { Socks5, Http };   // which protocol a listener serves
    void onNewConnection(QTcpServer *server, Proto proto);
    void sessionFinished(quint64 id);

    QTcpServer *m_httpServer = nullptr;
    QTcpServer *m_socksServer = nullptr;
    DnsClient *m_dns = nullptr;
    quint16 m_httpPort = 0;
    quint16 m_socksPort = 0;
    quint64 m_nextId = 1;
    Config m_config;
    QHash<quint64, GatewaySession *> m_sessions;

    mutable QMutex m_mutex;
    RuleEnginePtr m_rules;
};
