#include "ProxyGateway.h"
#include "DnsClient.h"
#include "Logger.h"
#include "ProxyConnector.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QPointer>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

static constexpr qint64 kMaxBuffered = 4 * 1024 * 1024;
static constexpr int kChunk = 64 * 1024;
static constexpr int kConnectTimeoutMs = 20000;

// What a target should be sent through: direct (no hops), a proxy/chain, or
// blocked. Mirrors the relay's handling of a rule's action, simplified to a
// single connection attempt (no redundancy failover yet).
namespace {

struct Route {
    bool blocked = false;
    QList<ProxyServer> hops;   // empty = direct
    QString text;              // for the log
};

Route routeFor(const RuleEnginePtr &rules, const ProxyGateway::Config &cfg,
               const Endpoint &target, QString *ruleName)
{
    Route r;
    if (cfg.mode == ProxyGateway::Mode::Direct || !rules) {
        r.text = QCoreApplication::translate("ProxyGateway", "Direct");
        return r;
    }
    // The chosen proxy/chain is gone: either refuse, or fall back to direct.
    const auto missing = [&](const QString &why) {
        if (cfg.onMissing == ProxyGateway::OnMissing::Direct) {
            r.hops.clear();
            r.text = QCoreApplication::translate("ProxyGateway", "Direct (%1)").arg(why);
        } else {
            r.blocked = true;
            r.text = why;
        }
    };

    // Where the route comes from: the fixed exit, or whatever the rules match.
    RuleAction action = cfg.exit;
    if (cfg.mode == ProxyGateway::Mode::Rules) {
        ConnectionRequest req;
        req.ip = target.ip;
        req.hostname = target.host;
        req.port = target.port;
        const RuleMatch match = rules->match(req);
        if (ruleName)
            *ruleName = match.ruleName;
        action = match.action;
    } else if (action.type != ActionType::Proxy && action.type != ActionType::Chain) {
        // Fixed-exit mode but nothing valid is chosen.
        missing(QCoreApplication::translate("ProxyGateway", "no exit proxy is selected"));
        return r;
    }
    const Profile &profile = rules->profile();
    switch (action.type) {
    case ActionType::Direct:
        r.text = QCoreApplication::translate("ProxyGateway", "Direct");
        break;
    case ActionType::Block:
        r.blocked = true;
        r.text = QCoreApplication::translate("ProxyGateway", "Block");
        break;
    case ActionType::Proxy:
        if (const ProxyServer *p = profile.proxyById(action.id)) {
            r.hops << *p;
            r.text = QString("%1:%2 %3").arg(p->address).arg(p->port).arg(proxyTypeToString(p->type));
        } else {
            missing(QCoreApplication::translate("ProxyGateway", "proxy #%1 is missing").arg(action.id));
        }
        break;
    case ActionType::Chain:
        if (const ProxyChain *c = profile.chainById(action.id)) {
            QList<ProxyServer> members;
            for (const ChainMember &m : c->members)
                if (m.enabled)
                    if (const ProxyServer *p = profile.proxyById(m.proxyId))
                        members << *p;
            if (members.isEmpty()) {
                missing(QCoreApplication::translate("ProxyGateway", "chain %1 is empty").arg(c->name));
            } else if (c->type == ChainType::Simple) {
                r.hops = members;
                r.text = QCoreApplication::translate("ProxyGateway", "Chain %1").arg(c->name);
            } else if (c->type == ChainType::LoadBalancing) {
                r.hops << members.at(int(QRandomGenerator::global()->bounded(members.size())));
                r.text = QCoreApplication::translate("ProxyGateway", "Chain %1").arg(c->name);
            } else {   // Redundancy: use the first member (no failover yet)
                r.hops << members.first();
                r.text = QCoreApplication::translate("ProxyGateway", "Chain %1").arg(c->name);
            }
        } else {
            missing(QCoreApplication::translate("ProxyGateway", "chain #%1 is missing").arg(action.id));
        }
        break;
    }
    return r;
}

} // namespace

// One accepted inbound connection: reads the client's SOCKS5 handshake, works
// out the target, opens the upstream through the chosen route and then pumps
// bytes both ways, like the relay does.
class GatewaySession : public QObject {
public:
    enum class Proto { Unknown, Socks5, Http };

    GatewaySession(ProxyGateway *gw, quint64 id, QTcpSocket *client, Proto proto)
        : QObject(gw), m_gw(gw), m_id(id), m_client(client), m_proto(proto)
    {
        m_client->setParent(this);
        m_peer = m_client->peerAddress().toString();
        m_start = QDateTime::currentMSecsSinceEpoch();
        connect(m_client, &QTcpSocket::readyRead, this, [this] { onClientData(); });
        connect(m_client, &QTcpSocket::disconnected, this, [this] { onSideClosed(m_client); });
        connect(m_client, &QTcpSocket::errorOccurred, this, [this] { onSideClosed(m_client); });
    }

    quint64 id() const { return m_id; }

    void start()
    {
        onClientData();   // in case the greeting arrived with the connection
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
    enum class Stage { Greeting, Auth, Request, HttpHeaders, Connecting, Open };

    void onClientData()
    {
        if (!m_client)
            return;
        m_buf += m_client->readAll();
        if (m_stage == Stage::Open) {
            pump(m_client, m_upstream, &m_sent);
            return;
        }
        bool progressed = true;
        while (progressed) {
            progressed = false;
            switch (m_stage) {
            case Stage::Greeting:    progressed = handleGreeting(); break;
            case Stage::Auth:        progressed = handleAuth(); break;
            case Stage::Request:     progressed = handleRequest(); break;
            case Stage::HttpHeaders: progressed = handleHttp(); break;
            default: break;
            }
        }
    }

    // First bytes decide the protocol: 0x05 is SOCKS5, anything else is treated
    // as HTTP (a request line like "CONNECT host:port ..." or "GET http://...").
    bool handleGreeting()
    {
        if (m_buf.isEmpty())
            return false;
        // A dedicated listener fixes the protocol; otherwise sniff the first byte.
        if (m_proto == Proto::Unknown)
            m_proto = quint8(m_buf.at(0)) == 0x05 ? Proto::Socks5 : Proto::Http;
        if (m_proto == Proto::Http) {
            m_stage = Stage::HttpHeaders;
            return true;
        }
        if (quint8(m_buf.at(0)) != 0x05) {
            fail(QCoreApplication::translate("ProxyGateway", "%1 - not a SOCKS5 client").arg(m_peer));
            return false;
        }
        if (m_buf.size() < 2)
            return false;
        const int n = quint8(m_buf.at(1));
        if (m_buf.size() < 2 + n)
            return false;
        const QByteArray methods = m_buf.mid(2, n);
        m_buf.remove(0, 2 + n);

        const bool needAuth = !m_gw->config().username.isEmpty();
        const char want = needAuth ? char(0x02) : char(0x00);
        if (!methods.contains(want)) {
            m_client->write(QByteArray::fromRawData("\x05\xFF", 2));   // no acceptable method
            fail(QCoreApplication::translate("ProxyGateway", "client offered no acceptable authentication method"));
            return false;
        }
        const char reply[2] = {0x05, want};
        m_client->write(QByteArray(reply, 2));
        m_stage = needAuth ? Stage::Auth : Stage::Request;
        return true;
    }

    // SOCKS5 username/password auth (RFC 1929): 0x01, ulen, user, plen, pass.
    bool handleAuth()
    {
        if (m_buf.size() < 2)
            return false;
        const int ulen = quint8(m_buf.at(1));
        if (m_buf.size() < 2 + ulen + 1)
            return false;
        const int plen = quint8(m_buf.at(2 + ulen));
        if (m_buf.size() < 2 + ulen + 1 + plen)
            return false;
        const QString user = QString::fromUtf8(m_buf.mid(2, ulen));
        const QString pass = QString::fromUtf8(m_buf.mid(2 + ulen + 1, plen));
        m_buf.remove(0, 2 + ulen + 1 + plen);

        const bool ok = user == m_gw->config().username && pass == m_gw->config().password;
        const char reply[2] = {0x01, char(ok ? 0x00 : 0x01)};
        m_client->write(QByteArray(reply, 2));
        if (!ok) {
            fail(QCoreApplication::translate("ProxyGateway", "%1 - wrong user name or password").arg(m_peer));
            return false;
        }
        m_stage = Stage::Request;
        return true;
    }

    // SOCKS5 request: 0x05, cmd, 0x00, atyp, addr, port.
    bool handleRequest()
    {
        if (m_buf.size() < 4)
            return false;
        const quint8 cmd = quint8(m_buf.at(1));
        const quint8 atyp = quint8(m_buf.at(3));
        int need = 4;
        QString host;
        QHostAddress ip;
        if (atyp == 0x01) {        // IPv4
            need = 4 + 4 + 2;
            if (m_buf.size() < need)
                return false;
            quint32 v4 = 0;
            for (int i = 0; i < 4; ++i)
                v4 = (v4 << 8) | quint8(m_buf.at(4 + i));
            ip = QHostAddress(v4);
        } else if (atyp == 0x04) { // IPv6
            need = 4 + 16 + 2;
            if (m_buf.size() < need)
                return false;
            Q_IPV6ADDR raw;
            for (int i = 0; i < 16; ++i)
                raw[i] = quint8(m_buf.at(4 + i));
            ip = QHostAddress(raw);
        } else if (atyp == 0x03) { // domain name
            const int len = quint8(m_buf.at(4));
            need = 4 + 1 + len + 2;
            if (m_buf.size() < need)
                return false;
            host = QString::fromUtf8(m_buf.mid(5, len));
        } else {
            sendSocksReply(0x08);   // address type not supported
            fail(QCoreApplication::translate("ProxyGateway", "%1 - unsupported SOCKS5 address type").arg(m_peer));
            return false;
        }
        const quint16 port = (quint8(m_buf.at(need - 2)) << 8) | quint8(m_buf.at(need - 1));
        m_buf.remove(0, need);

        if (cmd != 0x01) {   // only CONNECT; no BIND or UDP ASSOCIATE
            sendSocksReply(0x07); // command not supported
            fail(QCoreApplication::translate("ProxyGateway", "%1 - unsupported SOCKS5 command %2").arg(m_peer).arg(cmd));
            return false;
        }

        m_target.ip = ip;
        m_target.host = host;
        m_target.port = port;
        m_targetText = host.isEmpty() ? util::formatEndpoint(ip, port) : QString("%1:%2").arg(host).arg(port);
        m_pendingToUpstream = m_buf;   // any data the client sent ahead of our reply
        m_buf.clear();
        beginUpstream();
        return false;   // handshake consumed; upstream is async from here
    }

    // HTTP proxy request: "CONNECT host:port" (tunnel) or an absolute-URI
    // request ("GET http://host/path ...") that we forward to the origin.
    bool handleHttp()
    {
        const int end = m_buf.indexOf("\r\n\r\n");
        if (end < 0) {
            if (m_buf.size() > 64 * 1024)
                httpError("400 Bad Request", QCoreApplication::translate("ProxyGateway", "oversized request header"));
            return false;   // wait for the rest of the headers
        }
        const QByteArray head = m_buf.left(end);
        m_buf.remove(0, end + 4);

        const QList<QByteArray> lines = head.split('\n');
        const QByteArray requestLine = lines.value(0).trimmed();
        const QList<QByteArray> parts = requestLine.split(' ');
        if (parts.size() < 3) {
            httpError("400 Bad Request", QCoreApplication::translate("ProxyGateway", "malformed request line"));
            return false;
        }
        const QByteArray method = parts.at(0);
        const QByteArray uri = parts.at(1);
        const QByteArray version = parts.at(2);

        if (!checkHttpAuth(lines))
            return false;

        if (method.compare("CONNECT", Qt::CaseInsensitive) == 0) {
            const int colon = uri.lastIndexOf(':');
            if (colon < 0) {
                httpError("400 Bad Request", QCoreApplication::translate("ProxyGateway", "CONNECT without a port"));
                return false;
            }
            m_httpConnect = true;
            m_target.host = QString::fromUtf8(uri.left(colon));
            m_target.ip = QHostAddress(m_target.host);   // null if it is a name
            if (!m_target.ip.isNull())
                m_target.host.clear();
            m_target.port = uri.mid(colon + 1).toUShort();
            m_pendingToUpstream = m_buf;   // tunnelled bytes already sent (e.g. TLS hello)
            m_buf.clear();
        } else {
            const QUrl url = QUrl::fromEncoded(uri, QUrl::StrictMode);
            if (!url.isValid() || url.host().isEmpty() || url.scheme().compare("http", Qt::CaseInsensitive) != 0) {
                httpError("400 Bad Request", QCoreApplication::translate("ProxyGateway", "a proxy request must use an absolute http:// URL"));
                return false;
            }
            m_target.host = url.host();
            m_target.ip = QHostAddress(m_target.host);
            if (!m_target.ip.isNull())
                m_target.host.clear();
            m_target.port = url.port(80);
            QByteArray path = url.path(QUrl::FullyEncoded).toUtf8();
            if (path.isEmpty())
                path = "/";
            if (url.hasQuery())
                path += '?' + url.query(QUrl::FullyEncoded).toUtf8();
            // Rebuild the request in origin form and forward it, dropping the
            // proxy-only headers and closing after the response (one request per
            // upstream connection keeps this simple and avoids cross-host reuse).
            QByteArray req = method + ' ' + path + ' ' + version + "\r\n";
            for (int i = 1; i < lines.size(); ++i) {
                const QByteArray line = lines.at(i).endsWith('\r') ? lines.at(i).chopped(1) : lines.at(i);
                if (line.isEmpty())
                    continue;
                const QByteArray lower = line.toLower();
                if (lower.startsWith("proxy-connection:") || lower.startsWith("proxy-authorization:")
                    || lower.startsWith("connection:"))
                    continue;
                req += line + "\r\n";
            }
            req += "Connection: close\r\n\r\n";
            m_pendingToUpstream = req + m_buf;   // rewritten head + any body bytes already read
            m_buf.clear();
        }

        const QHostAddress &ip = m_target.ip;
        m_targetText = m_target.host.isEmpty() ? util::formatEndpoint(ip, m_target.port)
                                               : QString("%1:%2").arg(m_target.host).arg(m_target.port);
        beginUpstream();
        return false;
    }

    // Validates HTTP Proxy-Authorization when a user name is configured.
    bool checkHttpAuth(const QList<QByteArray> &lines)
    {
        if (m_gw->config().username.isEmpty())
            return true;
        QByteArray credentials;
        for (const QByteArray &raw : lines) {
            const QByteArray line = raw.trimmed();
            if (line.toLower().startsWith("proxy-authorization:")) {
                const QByteArray value = line.mid(line.indexOf(':') + 1).trimmed();
                if (value.toLower().startsWith("basic "))
                    credentials = QByteArray::fromBase64(value.mid(6).trimmed());
                break;
            }
        }
        const QByteArray expected =
            (m_gw->config().username + ':' + m_gw->config().password).toUtf8();
        if (credentials == expected && !credentials.isEmpty())
            return true;
        if (m_client) {
            m_client->write("HTTP/1.1 407 Proxy Authentication Required\r\n"
                            "Proxy-Authenticate: Basic realm=\"WinRoute\"\r\n"
                            "Content-Length: 0\r\nConnection: close\r\n\r\n");
        }
        fail(QCoreApplication::translate("ProxyGateway", "%1 - missing or wrong proxy credentials").arg(m_peer));
        return false;
    }

    void beginUpstream()
    {
        m_stage = Stage::Connecting;
        QString ruleName;
        const Route route = routeFor(m_gw->rules(), m_gw->config(), m_target, &ruleName);
        if (route.blocked) {
            rejectClient();   // refuse per the ruleset
            logNormal(QCoreApplication::translate("ProxyGateway", "%1 - %2 blocked (%3)").arg(m_peer, m_targetText, route.text));
            m_failed = true;
            finish();
            return;
        }
        m_actionText = route.text;
        logNormal(QCoreApplication::translate("ProxyGateway", "%1 - %2 via %3").arg(m_peer, m_targetText, route.text));

        // When the chosen proxy/chain cannot be connected, fall back to a direct
        // connection if the "when the selected proxy cannot connect" setting says
        // so (a direct route, with no hops, has nothing to fall back from).
        m_fallbackDirect = !route.hops.isEmpty()
                           && m_gw->config().onMissing == ProxyGateway::OnMissing::Direct;
        connectThrough(route.hops);
    }

    void connectThrough(const QList<ProxyServer> &hops)
    {
        m_upstream = new QTcpSocket(this);
        m_upstream->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        auto *connector = new ProxyConnector(m_upstream, hops, m_target, m_gw->dns(), kConnectTimeoutMs, this);
        connect(connector, &ProxyConnector::connected, this, [this, connector](const QByteArray &leftover) {
            connector->deleteLater();
            onUpstreamReady(leftover);
        });
        connect(connector, &ProxyConnector::failed, this, [this, connector](const QString &err) {
            connector->deleteLater();
            if (m_fallbackDirect) {   // proxy unreachable: try once more, directly
                m_fallbackDirect = false;
                logNormal(QCoreApplication::translate("ProxyGateway", "%1 - %2 proxy unreachable, connecting directly : %3").arg(m_peer, m_targetText, err));
                if (m_upstream)
                    m_upstream->deleteLater();
                connectThrough({});
                return;
            }
            failClient();
            logError(QCoreApplication::translate("ProxyGateway", "%1 - %2 failed : %3").arg(m_peer, m_targetText, err));
            m_failed = true;
            finish();
        });
        connector->start();
    }

    void onUpstreamReady(const QByteArray &leftover)
    {
        ackClient();
        m_stage = Stage::Open;

        connect(m_upstream, &QTcpSocket::readyRead, this, [this] { pump(m_upstream, m_client, &m_received); });
        connect(m_upstream, &QTcpSocket::bytesWritten, this, [this] { pump(m_client, m_upstream, &m_sent); });
        connect(m_client, &QTcpSocket::bytesWritten, this, [this] { pump(m_upstream, m_client, &m_received); });
        connect(m_upstream, &QTcpSocket::disconnected, this, [this] { onSideClosed(m_upstream); });
        connect(m_upstream, &QTcpSocket::errorOccurred, this, [this] { onSideClosed(m_upstream); });

        if (!leftover.isEmpty() && m_client) {
            m_client->write(leftover);   // downstream bytes the last proxy already sent
            m_received += leftover.size();
        }
        if (!m_pendingToUpstream.isEmpty()) {
            m_upstream->write(m_pendingToUpstream);   // tunnelled data / rewritten request
            m_sent += m_pendingToUpstream.size();
            m_pendingToUpstream.clear();
        }
        pump(m_client, m_upstream, &m_sent);
        pump(m_upstream, m_client, &m_received);
        if (m_upstream->state() != QAbstractSocket::ConnectedState)
            onSideClosed(m_upstream);
    }

    // Tell the client its tunnel is open (protocol-specific).
    void ackClient()
    {
        if (!m_client || m_replied)
            return;
        m_replied = true;
        if (m_proto == Proto::Socks5)
            sendSocksReply(0x00);                        // success, BND 0.0.0.0:0
        else if (m_httpConnect)
            m_client->write("HTTP/1.1 200 Connection Established\r\n\r\n");
        // An absolute-URI HTTP request needs no ack: the origin's response is
        // simply pumped back to the client.
    }

    // Refuse the connection (blocked by the ruleset).
    void rejectClient()
    {
        if (!m_client || m_replied)
            return;
        m_replied = true;
        if (m_proto == Proto::Socks5)
            sendSocksReply(0x02);                        // not allowed by ruleset
        else
            m_client->write("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    }

    // The upstream connection could not be made.
    void failClient()
    {
        if (!m_client || m_replied)
            return;
        m_replied = true;
        if (m_proto == Proto::Socks5)
            sendSocksReply(0x01);                        // general failure
        else
            m_client->write("HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    }

    // SOCKS5 reply with BND.ADDR = 0.0.0.0, BND.PORT = 0.
    void sendSocksReply(quint8 rep)
    {
        if (!m_client)
            return;
        const char reply[10] = {0x05, char(rep), 0x00, 0x01, 0, 0, 0, 0, 0, 0};
        m_client->write(QByteArray(reply, 10));
    }

    void httpError(const char *status, const QString &why)
    {
        if (m_client)
            m_client->write(QByteArray("HTTP/1.1 ") + status + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        fail(QCoreApplication::translate("ProxyGateway", "%1 - %2").arg(m_peer, why));
    }

    void pump(QTcpSocket *from, QTcpSocket *to, qint64 *counter)
    {
        if (m_stage != Stage::Open || !from || !to || to->state() != QAbstractSocket::ConnectedState)
            return;
        while (from->bytesAvailable() > 0 && to->bytesToWrite() < kMaxBuffered) {
            const QByteArray chunk = from->read(kChunk);
            if (chunk.isEmpty())
                break;
            to->write(chunk);
            *counter += chunk.size();
        }
    }

    void onSideClosed(QTcpSocket *side)
    {
        if (m_finished)
            return;
        QTcpSocket *other = side == m_client ? m_upstream.data() : m_client.data();
        if (m_stage != Stage::Open) {
            if (side == m_client)
                finish();   // client left during the handshake/connect
            return;
        }
        if (other) {
            pump(side, other, side == m_client ? &m_sent : &m_received);
            if (other->state() == QAbstractSocket::ConnectedState) {
                other->disconnectFromHost();
                if (other->state() != QAbstractSocket::UnconnectedState)
                    return;
            }
        }
        finish();
    }

    void fail(const QString &error)
    {
        if (m_finished)
            return;
        if (!error.isEmpty())
            logError(QCoreApplication::translate("ProxyGateway", "%1 : %2").arg(m_peer, error));
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
        if (m_stage == Stage::Open) {
            const qint64 life = (QDateTime::currentMSecsSinceEpoch() - m_start) / 1000;
            logNormal(QCoreApplication::translate("ProxyGateway", "%1 - %2 closed, %3 sent, %4 received, lifetime %5")
                          .arg(m_peer, m_targetText, util::formatBytes(m_sent), util::formatBytes(m_received), util::formatDuration(life)));
        }
        m_gw->sessionFinished(m_id);
        deleteLater();
    }

    ProxyGateway *m_gw;
    quint64 m_id;
    QPointer<QTcpSocket> m_client;
    QPointer<QTcpSocket> m_upstream;
    Endpoint m_target;
    QString m_peer;
    QString m_targetText;
    QString m_actionText;
    QByteArray m_buf;
    QByteArray m_pendingToUpstream;   // bytes to send upstream once connected
    Proto m_proto = Proto::Unknown;
    bool m_httpConnect = false;       // HTTP CONNECT tunnel (vs an absolute-URI request)
    bool m_fallbackDirect = false;    // on proxy connect failure, retry directly (once)
    Stage m_stage = Stage::Greeting;
    qint64 m_start = 0;
    qint64 m_sent = 0, m_received = 0;
    bool m_replied = false;
    bool m_failed = false;
    bool m_finished = false;
};

ProxyGateway::ProxyGateway(QObject *parent) : QObject(parent) {}

ProxyGateway::~ProxyGateway()
{
    stop();
}

bool ProxyGateway::start(const Config &config, QString *error)
{
    stop();
    m_config = config;

    m_dns = new DnsClient(this);
    if (!m_dns->bind(0))
        logError(QCoreApplication::translate("ProxyGateway", "Cannot bind the proxy server's DNS resolver socket"));

    // Bring up each enabled protocol on its own port.
    const auto listen = [&](Proto proto, bool enabled, quint16 port, QTcpServer **serverOut, quint16 *portOut) -> bool {
        if (!enabled)
            return true;
        auto *server = new QTcpServer(this);
        server->setMaxPendingConnections(1024);
        if (!server->listen(config.listenAddress, port)) {
            if (error)
                *error = QCoreApplication::translate("ProxyGateway", "Cannot start the proxy server on %1:%2 : %3")
                             .arg(config.listenAddress.toString()).arg(port).arg(server->errorString());
            delete server;
            return false;
        }
        *serverOut = server;
        *portOut = server->serverPort();
        connect(server, &QTcpServer::newConnection, this, [this, server, proto] { onNewConnection(server, proto); });
        logNormal(QCoreApplication::translate("ProxyGateway", "Proxy gateway (%1) listening on %2:%3")
                      .arg(proto == Proto::Http ? QStringLiteral("HTTP/HTTPS") : QStringLiteral("SOCKS5"),
                           config.listenAddress.toString())
                      .arg(*portOut));
        return true;
    };

    if (!listen(Proto::Http, config.httpEnabled, config.httpPort, &m_httpServer, &m_httpPort)
        || !listen(Proto::Socks5, config.socksEnabled, config.socksPort, &m_socksServer, &m_socksPort)) {
        stop();
        return false;
    }
    if (!m_httpServer && !m_socksServer) {
        if (error)
            *error = QCoreApplication::translate("ProxyGateway", "No proxy protocol is enabled");
        stop();
        return false;
    }
    return true;
}

void ProxyGateway::stop()
{
    for (QTcpServer **s : {&m_httpServer, &m_socksServer}) {
        if (*s) {
            (*s)->close();
            delete *s;
            *s = nullptr;
        }
    }
    const auto sessions = m_sessions;
    m_sessions.clear();
    for (GatewaySession *s : sessions)
        s->abort();
    if (m_dns) {
        delete m_dns;
        m_dns = nullptr;
    }
    m_httpPort = 0;
    m_socksPort = 0;
}

void ProxyGateway::onNewConnection(QTcpServer *server, Proto proto)
{
    const GatewaySession::Proto sp = proto == Proto::Http ? GatewaySession::Proto::Http : GatewaySession::Proto::Socks5;
    while (QTcpSocket *client = server->nextPendingConnection()) {
        const quint64 id = m_nextId++;
        auto *session = new GatewaySession(this, id, client, sp);
        m_sessions.insert(id, session);
        session->start();
    }
}

void ProxyGateway::sessionFinished(quint64 id)
{
    m_sessions.remove(id);
}

ProxyGateway::Config ProxyGateway::loadConfig()
{
    QSettings s = util::appSettings();
    Config c;
    c.enabled = s.value("proxyServer/enabled", false).toBool();
    const QString addr = s.value("proxyServer/listen").toString();
    c.listenAddress = addr.isEmpty() ? QHostAddress(QHostAddress::Any) : QHostAddress(addr);
    c.httpEnabled = s.value("proxyServer/httpEnabled", true).toBool();
    c.httpPort = quint16(s.value("proxyServer/httpPort", 8080).toUInt());
    c.socksEnabled = s.value("proxyServer/socksEnabled", true).toBool();
    c.socksPort = quint16(s.value("proxyServer/socksPort", 1080).toUInt());
    const QString mode = s.value("proxyServer/mode").toString();
    c.mode = mode == QLatin1String("direct") ? Mode::Direct
             : mode == QLatin1String("proxy") ? Mode::Proxy
                                              : Mode::Rules;
    c.onMissing = s.value("proxyServer/onMissing").toString() == QLatin1String("direct") ? OnMissing::Direct : OnMissing::Block;
    c.exit.type = s.value("proxyServer/exitType").toString() == QLatin1String("chain") ? ActionType::Chain : ActionType::Proxy;
    c.exit.id = s.value("proxyServer/exitId", 0).toInt();
    c.username = s.value("proxyServer/username").toString();
    c.password = s.value("proxyServer/password").toString();
    return c;
}

void ProxyGateway::saveConfig(const Config &c)
{
    QSettings s = util::appSettings();
    s.setValue("proxyServer/enabled", c.enabled);
    s.setValue("proxyServer/listen", c.listenAddress.toString());
    s.setValue("proxyServer/httpEnabled", c.httpEnabled);
    s.setValue("proxyServer/httpPort", c.httpPort);
    s.setValue("proxyServer/socksEnabled", c.socksEnabled);
    s.setValue("proxyServer/socksPort", c.socksPort);
    s.setValue("proxyServer/mode", c.mode == Mode::Direct ? "direct" : c.mode == Mode::Proxy ? "proxy" : "rules");
    s.setValue("proxyServer/onMissing", c.onMissing == OnMissing::Direct ? "direct" : "block");
    s.setValue("proxyServer/exitType", c.exit.type == ActionType::Chain ? "chain" : "proxy");
    s.setValue("proxyServer/exitId", c.exit.id);
    s.setValue("proxyServer/username", c.username);
    s.setValue("proxyServer/password", c.password);
}

void ProxyGateway::setRules(RuleEnginePtr rules)
{
    QMutexLocker lock(&m_mutex);
    m_rules = std::move(rules);
}

RuleEnginePtr ProxyGateway::rules() const
{
    QMutexLocker lock(&m_mutex);
    return m_rules;
}
