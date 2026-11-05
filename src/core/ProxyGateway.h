#pragma once

#include "RuleEngine.h"

#include <QHash>
#include <QHostAddress>
#include <QMutex>
#include <QObject>

#include <atomic>

class Capture;
class CertAuthority;
class QTcpServer;
class QTcpSocket;
class DnsClient;
class GatewaySession;

// Inbound proxy server: lets other machines reach the network through this one.
// It listens for HTTP (including CONNECT tunnels) and SOCKS5 clients and forwards each
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
    // Thread-safe versions for the UI: whether the gateway is listening, and
    // the bytes forwarded since the program started (sent = from the clients
    // out to the network, "up"; received = back to the clients, "down").
    bool running() const { return m_running.load(); }
    qint64 totalSent() const { return m_totalSent.load(); }
    qint64 totalReceived() const { return m_totalReceived.load(); }
    quint16 httpPort() const { return m_httpPort; }    // bound HTTP port, 0 if off
    quint16 socksPort() const { return m_socksPort; }  // bound SOCKS5 port, 0 if off

    void setRules(RuleEnginePtr rules);   // thread-safe
    // The packet capture the forwarded connections are recorded in (when it is
    // on and its filter passes them). Set once, before connections arrive.
    void setCapture(Capture *capture) { m_capture = capture; }
    // WinRoute's CA, for HTTPS decryption of captured connections (when the
    // capture's "Decrypt HTTPS" is on). The devices must trust its root.
    void setCertAuthority(CertAuthority *ca) { m_certAuthority = ca; }
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

    std::atomic<Capture *> m_capture{nullptr};
    std::atomic<CertAuthority *> m_certAuthority{nullptr};
    std::atomic<bool> m_running{false};
    std::atomic<qint64> m_totalSent{0};
    std::atomic<qint64> m_totalReceived{0};
};
