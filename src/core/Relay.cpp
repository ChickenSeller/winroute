#include "Relay.h"
#include "NetInterfaces.h"
#include "Capture.h"
#include "CertAuthority.h"
#include "DnsClient.h"
#include "Logger.h"
#include "UdpRelaySession.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QPointer>
#include <QRandomGenerator>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QtEndian>

#include <winsock2.h>
#include <ws2tcpip.h>

static constexpr qint64 kMaxBuffered = 4 * 1024 * 1024;
static constexpr int kChunk = 64 * 1024;
static constexpr int kConnectTimeoutMs = 20000;

static QString bytesText(qint64 n)
{
    if (n < 1024)
        return QCoreApplication::translate("Relay", "%1 bytes").arg(n);
    return QCoreApplication::translate("Relay", "%1 bytes (%2)").arg(n).arg(util::formatBytes(n));
}

// A QTcpServer that hands every accepted connection to the relay, which decides
// (before any byte is read) whether to terminate TLS for HTTPS decryption.
class RelayServer : public QTcpServer {
public:
    explicit RelayServer(Relay *relay, QObject *parent = nullptr) : QTcpServer(parent), m_relay(relay) {}

protected:
    void incomingConnection(qintptr socketDescriptor) override { m_relay->incomingConnection(socketDescriptor); }

private:
    Relay *m_relay;
};

// The application's local endpoint for an accepted socket, read straight from
// the OS so no data is consumed from the socket's buffer before we decide how
// to wrap it.
static bool peerEndpoint(qintptr fd, QHostAddress *addr, quint16 *port)
{
    sockaddr_storage ss{};
    int len = int(sizeof(ss));
    if (::getpeername(SOCKET(fd), reinterpret_cast<sockaddr *>(&ss), &len) != 0)
        return false;
    addr->setAddress(reinterpret_cast<sockaddr *>(&ss));
    if (ss.ss_family == AF_INET)
        *port = qFromBigEndian<quint16>(reinterpret_cast<sockaddr_in *>(&ss)->sin_port);
    else if (ss.ss_family == AF_INET6)
        *port = qFromBigEndian<quint16>(reinterpret_cast<sockaddr_in6 *>(&ss)->sin6_port);
    else
        return false;
    return true;
}

// One connection attempt: the hops to go through (empty = direct).
struct Attempt {
    QList<ProxyServer> hops;
    int timeoutMs = kConnectTimeoutMs;
    int redundancyProxyId = 0;   // set for redundancy chain members
    int recheckSec = 0;
};

class RelaySession : public QObject {
public:
    RelaySession(Relay *relay, quint64 id, QTcpSocket *client, const RedirectInfo &info, bool mitm = false)
        : QObject(relay), m_relay(relay), m_id(id), m_client(client), m_info(info), m_mitm(mitm)
    {
        m_client->setParent(this);
        m_start = QDateTime::currentMSecsSinceEpoch();
        m_target.port = info.dstPort;
        m_target.host = info.hostname;
        if (!info.fakeIp)
            m_target.ip = info.dstAddr;
        m_targetText = info.hostname.isEmpty() ? util::formatEndpoint(info.dstAddr, info.dstPort)
                                               : QString("%1:%2").arg(info.hostname).arg(info.dstPort);
        connect(m_client, &QTcpSocket::readyRead, this, [this] { pump(m_client, m_upstream, &m_sent); });
        connect(m_client, &QTcpSocket::bytesWritten, this, [this] { pump(m_upstream, m_client, &m_received); });
        connect(m_client, &QTcpSocket::disconnected, this, [this] { onSideClosed(m_client); });
        connect(m_client, &QTcpSocket::errorOccurred, this, [this] { onSideClosed(m_client); });
    }

    quint64 id() const { return m_id; }
    qint64 sent() const { return m_sent; }
    qint64 received() const { return m_received; }
    bool trafficChanged()
    {
        const bool changed = m_sent != m_lastSent || m_received != m_lastReceived;
        m_lastSent = m_sent;
        m_lastReceived = m_received;
        return changed;
    }

    void start()
    {
        RuleEnginePtr rules = m_relay->rules();
        if (!rules) {
            finishWithError(QCoreApplication::translate("Relay", "no active profile"));
            return;
        }
        const Profile &profile = rules->profile();
        const RuleAction &action = m_info.match.action;
        switch (action.type) {
        case ActionType::Direct:
        case ActionType::Block:
            m_actionText = QCoreApplication::translate("Relay", "Direct");
            m_attempts << Attempt{};
            break;
        case ActionType::Proxy: {
            const ProxyServer *p = profile.proxyById(action.id);
            if (!p) {
                finishWithError(QCoreApplication::translate("Relay", "proxy server #%1 used by rule %2 does not exist").arg(action.id).arg(m_info.match.ruleName));
                return;
            }
            m_actionText = QString("%1:%2 %3").arg(p->address).arg(p->port).arg(proxyTypeToString(p->type));
            m_attempts << Attempt{{*p}};
            break;
        }
        case ActionType::Chain: {
            const ProxyChain *c = profile.chainById(action.id);
            if (!c) {
                finishWithError(QCoreApplication::translate("Relay", "proxy chain #%1 used by rule %2 does not exist").arg(action.id).arg(m_info.match.ruleName));
                return;
            }
            m_actionText = QCoreApplication::translate("Relay", "Chain %1").arg(c->name);
            QList<ProxyServer> members;
            for (const ChainMember &m : c->members)
                if (m.enabled)
                    if (const ProxyServer *p = profile.proxyById(m.proxyId))
                        members << *p;
            if (members.isEmpty()) {
                finishWithError(QCoreApplication::translate("Relay", "proxy chain %1 has no enabled proxy servers").arg(c->name));
                return;
            }
            if (c->type == ChainType::Simple) {
                m_attempts << Attempt{members};
            } else if (c->type == ChainType::Redundancy) {
                QList<Attempt> healthy, failed;
                for (const ProxyServer &p : members) {
                    Attempt a{{p}, qMax(1, c->redundancyTimeout) * 1000, p.id, c->redundancyRecheck ? c->redundancyRecheckSec : 0};
                    (m_relay->proxyFailed(p.id) ? failed : healthy) << a;
                }
                m_attempts << healthy << failed;
                if (c->redundancyDirectFallback)
                    m_attempts << Attempt{};
            } else {
                int index;
                if (c->lbSamePidSameProxy)
                    index = int(m_info.pid % quint32(members.size()));
                else
                    index = int(QRandomGenerator::global()->bounded(members.size()));
                m_attempts << Attempt{{members[index]}};
            }
            break;
        }
        }

        if (!action.interfaceId.isEmpty() && action.type != ActionType::Block)
            m_actionText = QCoreApplication::translate("Relay", "%1 via %2").arg(m_actionText, netif::displayName(action.interfaceId));

        ConnectionEvent ev;
        ev.id = m_id;
        ev.pid = m_info.pid;
        ev.appName = m_info.appName;
        ev.appPath = m_info.appPath;
        ev.target = m_targetText;
        ev.ruleName = m_info.match.ruleName;
        ev.actionText = m_actionText;
        ev.actionType = m_info.match.action.type;
        ev.v6 = m_info.srcAddr.protocol() == QAbstractSocket::IPv6Protocol;
        ev.startMs = m_start;
        emit m_relay->opened(ev);
        m_announced = true;
        noteCapture();
        logVerbose(QCoreApplication::translate("Relay", "%1 - %2 matching %3 rule : using %4").arg(m_info.appName, m_targetText, m_info.match.ruleName,
                                                                       action.type == ActionType::Direct ? QCoreApplication::translate("Relay", "direct connection") : m_actionText));
        // Start the client-side TLS handshake now, before the event loop runs
        // again, so the ClientHello is read by the SSL engine and not buffered
        // as plaintext. The upstream handshake follows once the tunnel is up.
        if (m_mitm)
            startClientTls();
        if (!m_finished)
            nextAttempt();
    }

    void abort()
    {
        if (m_client)
            m_client->abort();
        if (m_upstream)
            m_upstream->abort();
        finish();
    }

private:
    void nextAttempt()
    {
        if (m_attemptIndex >= m_attempts.size()) {
            finishWithError(m_lastError);
            return;
        }
        const Attempt a = m_attempts[m_attemptIndex++];
        delete m_upstream;
        // For HTTPS decryption the upstream leg is a TLS client; the proxy
        // handshake still runs in the clear on it until startClientEncryption().
        m_upstream = m_mitm ? new QSslSocket(this) : new QTcpSocket(this);
        m_upstream->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        auto *connector = new ProxyConnector(m_upstream, a.hops, m_target, m_relay->dns(), a.timeoutMs, this);
        connector->setInterface(m_info.match.action.interfaceId);
        QObject::connect(connector, &ProxyConnector::connected, this, [this, connector, a](const QByteArray &leftover) {
            connector->deleteLater();
            const QString &iface = m_info.match.action.interfaceId;
            if (!iface.isEmpty() && connector->boundInterface().isEmpty())
                logNormal(QCoreApplication::translate("Relay", "%1 - %2 network adapter %3 is not available, connected through the system's default adapter")
                              .arg(m_info.appName, m_targetText, netif::displayName(iface)));
            onUpstreamReady(a, leftover);
        });
        QObject::connect(connector, &ProxyConnector::failed, this, [this, connector, a](const QString &err) {
            connector->deleteLater();
            m_lastError = err;
            if (a.redundancyProxyId) {
                m_relay->markProxyFailed(a.redundancyProxyId, a.recheckSec);
                if (m_attemptIndex < m_attempts.size())
                    logNormal(QCoreApplication::translate("Relay", "%1 - %2 %3, trying the next proxy in the chain").arg(m_info.appName, m_targetText, err));
            }
            if (m_client && m_client->state() == QAbstractSocket::ConnectedState)
                nextAttempt();
            else
                finish();
        });
        connector->start();
    }

    void onUpstreamReady(const Attempt &a, const QByteArray &leftover)
    {
        if (a.hops.isEmpty())
            logNormal(QCoreApplication::translate("Relay", "%1 - %2 open directly").arg(m_info.appName, m_targetText));
        else if (a.hops.size() == 1)
            logNormal(QCoreApplication::translate("Relay", "%1 - %2 open through proxy %3:%4 %5")
                          .arg(m_info.appName, m_targetText, a.hops[0].address)
                          .arg(a.hops[0].port)
                          .arg(proxyTypeToString(a.hops[0].type)));
        else
            logNormal(QCoreApplication::translate("Relay", "%1 - %2 open through %3").arg(m_info.appName, m_targetText, m_actionText));
        if (!a.hops.isEmpty() && a.hops.size() == 1 && m_attempts.size() > 1)
            m_actionText = QString("%1 (%2:%3)").arg(m_actionText, a.hops[0].address).arg(a.hops[0].port);
        m_open = true;
        emit m_relay->statusChanged(m_id, "Open");

        connect(m_upstream, &QTcpSocket::disconnected, this, [this] { onSideClosed(m_upstream); });
        connect(m_upstream, &QTcpSocket::errorOccurred, this, [this] { onSideClosed(m_upstream); });

        if (m_mitm) {
            // The TCP/proxy tunnel is up; now do TLS to the real server. Data is
            // pumped only once both the client and upstream handshakes finish.
            startUpstreamTls(leftover);
            return;
        }

        connect(m_upstream, &QTcpSocket::readyRead, this, [this] { pump(m_upstream, m_client, &m_received); });
        connect(m_upstream, &QTcpSocket::bytesWritten, this, [this] { pump(m_client, m_upstream, &m_sent); });
        if (!leftover.isEmpty() && m_client) {
            m_client->write(leftover);
            m_received += leftover.size();
            if (m_relay->m_capture && m_relay->m_capture->isEnabled())
                m_relay->m_capture->append(m_id, CaptureDir::Down, leftover, QDateTime::currentMSecsSinceEpoch());
        }
        pump(m_client, m_upstream, &m_sent);
        pump(m_upstream, m_client, &m_received);
        if (m_upstream->state() != QAbstractSocket::ConnectedState)
            onSideClosed(m_upstream);
    }

    // Presents our leaf certificate to the application and begins the TLS
    // server handshake on the client side.
    void startClientTls()
    {
        auto *ssl = qobject_cast<QSslSocket *>(m_client.data());
        if (!ssl || !m_relay->m_certAuthority) {
            finishWithError(QCoreApplication::translate("Relay", "HTTPS decryption is not available"));
            return;
        }
        const QString host = m_info.hostname.isEmpty() ? m_info.dstAddr.toString() : m_info.hostname;
        const CertAuthority::Leaf leaf = m_relay->m_certAuthority->leafFor(host);
        const QSslCertificate cert(leaf.certPem, QSsl::Pem);
        const QSslKey key(leaf.keyPem, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        if (cert.isNull() || key.isNull()) {
            finishWithError(QCoreApplication::translate("Relay", "could not create a certificate for %1").arg(host));
            return;
        }
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        cfg.setLocalCertificate(cert);
        cfg.setPrivateKey(key);
        cfg.setAllowedNextProtocols({QByteArrayLiteral("http/1.1")});
        ssl->setSslConfiguration(cfg);
        connect(ssl, &QSslSocket::encrypted, this, [this] {
            m_clientTls = true;
            beginTunnel();
        });
        ssl->startServerEncryption();
    }

    // Begins the TLS client handshake to the real server, verifying its
    // certificate against the system roots (a MITM upstream is refused).
    void startUpstreamTls(const QByteArray &leftover)
    {
        auto *ssl = qobject_cast<QSslSocket *>(m_upstream.data());
        if (!ssl) {
            finishWithError(QCoreApplication::translate("Relay", "HTTPS decryption is not available"));
            return;
        }
        if (!leftover.isEmpty()) {
            finishWithError(QCoreApplication::translate("Relay", "unexpected data before the TLS handshake"));
            return;
        }
        const QString host = m_info.hostname.isEmpty() ? m_info.dstAddr.toString() : m_info.hostname;
        ssl->setPeerVerifyName(host);
        QSslConfiguration cfg = ssl->sslConfiguration();
        cfg.setAllowedNextProtocols({QByteArrayLiteral("http/1.1")});
        ssl->setSslConfiguration(cfg);
        connect(ssl, &QSslSocket::encrypted, this, [this] {
            m_upstreamTls = true;
            beginTunnel();
        });
        connect(ssl, &QSslSocket::sslErrors, this, [this](const QList<QSslError> &errs) {
            QStringList msgs;
            for (const QSslError &e : errs)
                msgs << e.errorString();
            finishWithError(QCoreApplication::translate("Relay", "the server's certificate was rejected: %1")
                                .arg(msgs.join(QStringLiteral("; "))));
        });
        ssl->startClientEncryption();
    }

    // Both TLS legs are up: wire the plaintext pumps and start moving data. The
    // bytes handed to the capture store are now decrypted.
    void beginTunnel()
    {
        if (m_finished || m_tunnel || !m_clientTls || !m_upstreamTls)
            return;
        m_tunnel = true;
        connect(m_upstream, &QTcpSocket::readyRead, this, [this] { pump(m_upstream, m_client, &m_received); });
        connect(m_upstream, &QTcpSocket::bytesWritten, this, [this] { pump(m_client, m_upstream, &m_sent); });
        pump(m_client, m_upstream, &m_sent);
        pump(m_upstream, m_client, &m_received);
    }

    void pump(QTcpSocket *from, QTcpSocket *to, qint64 *counter)
    {
        if (!m_open || !from || !to || to->state() != QAbstractSocket::ConnectedState)
            return;
        // For a decrypted tunnel, hold off until both TLS handshakes are done.
        if (m_mitm && !m_tunnel)
            return;
        while (from->bytesAvailable() > 0 && to->bytesToWrite() < kMaxBuffered) {
            const QByteArray chunk = from->read(kChunk);
            if (chunk.isEmpty())
                break;
            to->write(chunk);
            *counter += chunk.size();
            if (m_relay->m_capture && m_relay->m_capture->isEnabled())
                m_relay->m_capture->append(m_id, from == m_client ? CaptureDir::Up : CaptureDir::Down,
                                           chunk, QDateTime::currentMSecsSinceEpoch(), m_mitm);
        }
    }

    // Registers this connection with the capture store (no-op unless capture is
    // on). Called once, right after the "opened" event.
    void noteCapture()
    {
        Capture *cap = m_relay->m_capture;
        if (!cap || !cap->isEnabled())
            return;
        const QHostAddress dip = m_info.fakeIp ? QHostAddress() : m_info.dstAddr;
        if (!cap->shouldCapture(m_info.appName, m_info.appPath, m_info.hostname, dip, m_info.dstPort))
            return;
        CaptureMeta cm;
        cm.id = m_id;
        cm.pid = m_info.pid;
        cm.appName = m_info.appName;
        cm.appPath = m_info.appPath;
        cm.target = m_targetText;
        cm.ruleName = m_info.match.ruleName;
        cm.actionText = m_actionText;
        cm.srcAddr = m_info.srcAddr;
        cm.srcPort = m_info.srcPort;
        cm.dstAddr = m_info.dstAddr;   // may be a fake-DNS IP; named via NRB on export
        cm.dstPort = m_info.dstPort;
        cm.hostname = m_info.hostname;
        cm.v6 = m_info.srcAddr.protocol() == QAbstractSocket::IPv6Protocol;
        cm.startMs = m_start;
        cap->note(cm);
    }

    // Qt sockets cannot half-close, so when one side finishes we flush what is
    // pending to the other side and then close it as well.
    void onSideClosed(QTcpSocket *side)
    {
        if (m_finished)
            return;
        QTcpSocket *other = side == m_client ? m_upstream.data() : m_client.data();
        if (!m_open) {
            if (side == m_client) {
                // The application went away while we were still connecting.
                finish();
            }
            return;
        }
        if (other) {
            pump(side, other, side == m_client ? &m_sent : &m_received);
            if (other->state() == QAbstractSocket::ConnectedState) {
                other->disconnectFromHost();
                if (other->state() != QAbstractSocket::UnconnectedState)
                    return;   // wait for "disconnected" of the other side
            }
        }
        finish();
    }

    void finishWithError(const QString &err)
    {
        if (m_finished)
            return;
        logError(QCoreApplication::translate("Relay", "%1 - %2 error : %3").arg(m_info.appName, m_targetText, err));
        if (!m_announced) {
            // Errors before the "opened" event (missing proxy etc.)
            ConnectionEvent ev;
            ev.id = m_id;
            ev.pid = m_info.pid;
            ev.appName = m_info.appName;
            ev.appPath = m_info.appPath;
            ev.target = m_targetText;
            ev.ruleName = m_info.match.ruleName;
            ev.actionText = m_actionText;
        ev.actionType = m_info.match.action.type;
            ev.v6 = m_info.srcAddr.protocol() == QAbstractSocket::IPv6Protocol;
            ev.startMs = m_start;
            emit m_relay->opened(ev);
        }
        emit m_relay->statusChanged(m_id, "Error : " + err);
        if (m_client)
            m_client->abort();
        m_failed = true;
        finish();
    }

    void finish()
    {
        if (m_finished)
            return;
        m_finished = true;
        if (m_open) {
            const qint64 life = (QDateTime::currentMSecsSinceEpoch() - m_start) / 1000;
            logNormal(QCoreApplication::translate("Relay", "%1 - %2 close, %3 sent, %4 received, lifetime %5")
                          .arg(m_info.appName, m_targetText, bytesText(m_sent), bytesText(m_received), util::formatDuration(life)));
        } else if (!m_failed) {
            logVerbose(QCoreApplication::translate("Relay", "%1 - %2 closed by the application before the connection was established")
                           .arg(m_info.appName, m_targetText));
        }
        if (m_relay->m_capture)
            m_relay->m_capture->finish(m_id, QDateTime::currentMSecsSinceEpoch());
        m_relay->sessionFinished(m_id);
        emit m_relay->closed(m_id, m_sent, m_received);
        deleteLater();
    }

    Relay *m_relay;
    quint64 m_id;
    QPointer<QTcpSocket> m_client;
    QPointer<QTcpSocket> m_upstream;
    RedirectInfo m_info;
    Endpoint m_target;
    QString m_targetText;
    QString m_actionText;
    QString m_lastError;
    QList<Attempt> m_attempts;
    int m_attemptIndex = 0;
    qint64 m_start = 0;
    qint64 m_sent = 0, m_received = 0;
    qint64 m_lastSent = 0, m_lastReceived = 0;
    bool m_open = false;
    bool m_failed = false;
    bool m_finished = false;
    bool m_announced = false;
    bool m_mitm = false;        // terminate TLS to decrypt this connection
    bool m_clientTls = false;   // client-side handshake finished
    bool m_upstreamTls = false; // upstream handshake finished
    bool m_tunnel = false;      // both handshakes done; plaintext is flowing
};

Relay::Relay(FlowTable *flows, QObject *parent)
    : QObject(parent), m_flows(flows), m_trafficTimer(this), m_expireTimer(this)
{
    // The timers are parented so that moveToThread() takes them to the relay thread.
    qRegisterMetaType<ConnectionEvent>();
    qRegisterMetaType<TrafficSample>();
    qRegisterMetaType<QList<TrafficSample>>();
    qRegisterMetaType<UdpDatagram>();
}

Relay::~Relay()
{
    closeAll();
}

bool Relay::listen(QString *error)
{
    m_dns = new DnsClient(this);
    if (!m_dns->bind(0))
        logError(QCoreApplication::translate("Relay", "Cannot bind the DNS resolver socket"));

    m_server4 = new RelayServer(this, this);
    m_server4->setMaxPendingConnections(1024);
    if (!m_server4->listen(QHostAddress::AnyIPv4, 0)) {
        *error = QCoreApplication::translate("Relay", "Cannot start the local relay: ") + m_server4->errorString();
        return false;
    }
    m_port = m_server4->serverPort();
    m_server6 = new RelayServer(this, this);
    m_server6->setMaxPendingConnections(1024);
    m_hasV6 = m_server6->listen(QHostAddress::AnyIPv6, m_port);
    if (!m_hasV6)
        logVerbose(QCoreApplication::translate("Relay", "IPv6 relay is not available: ") + m_server6->errorString());

    connect(&m_trafficTimer, &QTimer::timeout, this, &Relay::emitTraffic);
    m_trafficTimer.start(500);
    connect(&m_expireTimer, &QTimer::timeout, this, [this] { m_flows->expire(60000); });
    m_expireTimer.start(10000);
    return true;
}

void Relay::incomingConnection(qintptr fd)
{
    QHostAddress peerAddr;
    quint16 peerPort = 0;
    if (!peerEndpoint(fd, &peerAddr, &peerPort)) {
        ::closesocket(SOCKET(fd));
        return;
    }
    const auto info = m_flows->take(peerPort);
    // Only connections from this machine that the interceptor announced are relayed.
    const QHostAddress peer = util::normalize(peerAddr);
    const QHostAddress src = util::normalize(info ? info->srcAddr : QHostAddress());
    const bool local = peer.isLoopback() || src.isNull() || src.isEqual(QHostAddress::AnyIPv4)
                       || src.isEqual(QHostAddress::AnyIPv6) || src.isEqual(peer);
    if (!info || !local) {
        logDebug(QCoreApplication::translate("Relay", "Relay: unexpected connection from %1 dropped").arg(util::formatEndpoint(peer, peerPort)));
        ::closesocket(SOCKET(fd));
        return;
    }

    // Decide whether to terminate TLS for HTTPS decryption. Only when capture
    // and decryption are both on, a CA is wired, the target is the standard
    // HTTPS port, and the connection passes the capture filter. Committed here,
    // before any byte is read, because QSslSocket must own the ClientHello.
    const bool mitm = m_capture && m_capture->isEnabled() && m_capture->decrypt() && m_certAuthority
                      && info->dstPort == 443
                      && m_capture->shouldCapture(info->appName, info->appPath, info->hostname,
                                                  info->fakeIp ? QHostAddress() : info->dstAddr, info->dstPort);

    QTcpSocket *s = mitm ? new QSslSocket : new QTcpSocket;
    if (!s->setSocketDescriptor(fd, QAbstractSocket::ConnectedState)) {
        delete s;
        ::closesocket(SOCKET(fd));
        return;
    }
    s->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    auto *session = new RelaySession(this, m_nextId++, s, *info, mitm);
    m_sessions.insert(session->id(), session);
    session->start();
}

void Relay::sessionFinished(quint64 id)
{
    m_sessions.remove(id);
}

void Relay::udpOut(const UdpDatagram &dg)
{
    if (UdpRelaySession *s = m_udpSessions.value(dg.id)) {
        s->send(dg);
        return;
    }
    // proxyId 0: a direct rule bound to a network adapter, sent from there.
    const bool direct = dg.proxyId == 0;
    RuleEnginePtr rules = this->rules();
    const ProxyServer *p = rules && !direct ? rules->profile().proxyById(dg.proxyId) : nullptr;
    if (!direct && !p) {
        logError(QCoreApplication::translate("Relay", "%1 - UDP proxy #%2 used by rule %3 does not exist")
                     .arg(dg.appName).arg(dg.proxyId).arg(dg.ruleName));
        return;
    }
    // The constructor takes the first datagram, so do not send it again here.
    auto *s = new UdpRelaySession(this, dg, direct ? ProxyServer{} : *p, this);
    // With a backend injector (WinDivert), the session delivers replies and
    // bypassed datagrams through it instead of the NetFilter driver API.
    if (m_udpInjector.deliver) {
        const QByteArray local = dg.localEndpoint;
        const UdpInjector inj = m_udpInjector;
        s->setSinks(
            [inj, local](const QByteArray &appRemote, const QByteArray &payload) { inj.deliver(local, appRemote, payload); },
            [inj](const UdpDatagram &d) { inj.bypass(d); });
    }
    m_udpSessions.insert(dg.id, s);

    ConnectionEvent ev;
    ev.id = dg.id;
    ev.pid = dg.pid;
    ev.appName = dg.appName;
    ev.appPath = dg.appPath;
    ev.target = UdpRelaySession::targetText(dg.remote, dg.hostname);
    ev.ruleName = dg.ruleName;
    ev.actionText = s->actionText();
    ev.actionType = direct ? ActionType::Direct : ActionType::Proxy;
    ev.udp = true;
    ev.v6 = dg.v6;
    ev.startMs = QDateTime::currentMSecsSinceEpoch();
    emit opened(ev);
}

void Relay::closeUdp(quint64 id)
{
    QMetaObject::invokeMethod(this, [this, id] {
        if (UdpRelaySession *s = m_udpSessions.value(id))
            s->close();
    });
}

void Relay::udpSessionFinished(quint64 id, qint64 sent, qint64 received)
{
    m_udpSessions.remove(id);
    emit closed(id, sent, received);
}

void Relay::emitTraffic()
{
    QList<TrafficSample> samples;
    for (RelaySession *s : std::as_const(m_sessions))
        if (s->trafficChanged())
            samples << TrafficSample{s->id(), s->sent(), s->received()};
    for (UdpRelaySession *s : std::as_const(m_udpSessions))
        if (s->trafficChanged())
            samples << TrafficSample{s->id(), s->sent(), s->received()};
    if (!samples.isEmpty())
        emit traffic(samples);
}

void Relay::setRules(RuleEnginePtr rules)
{
    QMutexLocker lock(&m_mutex);
    m_rules = std::move(rules);
}

RuleEnginePtr Relay::rules() const
{
    QMutexLocker lock(&m_mutex);
    return m_rules;
}

void Relay::closeConnection(quint64 id)
{
    QMetaObject::invokeMethod(this, [this, id] {
        if (RelaySession *s = m_sessions.value(id))
            s->abort();
    });
}

void Relay::closeAll()
{
    auto doClose = [this] {
        const auto sessions = m_sessions.values();
        for (RelaySession *s : sessions)
            s->abort();
        const auto udp = m_udpSessions.values();
        for (UdpRelaySession *s : udp)
            s->close();
    };
    if (thread() == QThread::currentThread())
        doClose();
    else
        QMetaObject::invokeMethod(this, doClose, Qt::BlockingQueuedConnection);
}

bool Relay::proxyFailed(int proxyId) const
{
    QMutexLocker lock(&m_mutex);
    return m_failedUntil.value(proxyId, 0) > QDateTime::currentMSecsSinceEpoch();
}

void Relay::markProxyFailed(int proxyId, int recheckSec)
{
    QMutexLocker lock(&m_mutex);
    // Without "recheck" a failed proxy stays failed until the status is reset.
    const qint64 until = recheckSec > 0 ? QDateTime::currentMSecsSinceEpoch() + qint64(recheckSec) * 1000
                                        : std::numeric_limits<qint64>::max();
    m_failedUntil.insert(proxyId, until);
}

void Relay::resetProxyFailures()
{
    QMutexLocker lock(&m_mutex);
    m_failedUntil.clear();
}
