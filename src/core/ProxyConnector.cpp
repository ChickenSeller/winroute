#include "ProxyConnector.h"
#include "DnsClient.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QUrl>
#include <QtEndian>

QString Endpoint::toString() const
{
    if (!host.isEmpty())
        return QString("%1:%2").arg(host).arg(port);
    return util::formatEndpoint(ip, port);
}

ProxyConnector::ProxyConnector(QTcpSocket *socket, const QList<ProxyServer> &hops, const Endpoint &target,
                               DnsClient *dns, int timeoutMs, QObject *parent)
    : QObject(parent), m_socket(socket), m_hops(hops), m_dns(dns)
{
    for (const auto &h : hops) {
        Endpoint e;
        const QHostAddress literal(h.address);
        if (literal.isNull())
            e.host = h.address;
        else
            e.ip = literal;
        e.port = h.port;
        m_endpoints << e;
    }
    m_endpoints << target;

    m_timer.setSingleShot(true);
    m_timer.setInterval(timeoutMs);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (m_stage == Stage::Connecting || m_stage == Stage::Resolving)
            fail(m_hops.isEmpty() ? QCoreApplication::translate("ProxyConnector", "Could not connect to %1 - connection timeout").arg(m_endpoints[0].toString())
                                  : QCoreApplication::translate("ProxyConnector", "Could not connect to proxy %1 - connection timeout").arg(proxyName(0)));
        else
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - proxy handshake timeout").arg(proxyName(m_hop)));
    });
}

QString ProxyConnector::proxyName(int hop) const
{
    if (hop < 0 || hop >= m_hops.size())
        return m_endpoints.last().toString();
    return QString("%1:%2").arg(m_hops[hop].address).arg(m_hops[hop].port);
}

bool ProxyConnector::hopAcceptsName(int hop) const
{
    const ProxyServer &p = m_hops[hop];
    if (p.type == ProxyType::Socks4)
        return p.socks4a;
    return true;
}

void ProxyConnector::start()
{
    m_timer.start();
    resolveNext(0);
}

// Resolves, in order, every endpoint that needs a local address:
// the first one (we open a TCP connection to it) and any next-hop target
// whose proxy cannot resolve names remotely.
void ProxyConnector::resolveNext(int index)
{
    for (; index < m_endpoints.size(); ++index) {
        Endpoint &e = m_endpoints[index];
        if (!e.ip.isNull())
            continue;
        const bool needed = index == 0 || !hopAcceptsName(index - 1);
        if (!needed)
            continue;
        if (e.host.isEmpty()) {
            fail(QCoreApplication::translate("ProxyConnector", "No target address"));
            return;
        }
        if (!m_dns) {
            fail(QCoreApplication::translate("ProxyConnector", "No resolver available"));
            return;
        }
        const QString host = e.host;
        QPointer<ProxyConnector> self(this);
        m_dns->resolve(host, [self, index, host](const QList<QHostAddress> &addrs, const QString &err) {
            if (!self || self->m_finished)
                return;
            if (addrs.isEmpty()) {
                self->fail(QCoreApplication::translate("ProxyConnector", "Could not resolve %1 - %2").arg(host, err));
                return;
            }
            self->m_endpoints[index].ip = addrs.first();
            self->resolveNext(index + 1);
        });
        return;
    }
    connectFirst();
}

void ProxyConnector::connectFirst()
{
    if (!m_socket) {
        fail(QCoreApplication::translate("ProxyConnector", "Connection closed"));
        return;
    }
    m_stage = Stage::Connecting;
    connect(m_socket, &QTcpSocket::connected, this, &ProxyConnector::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ProxyConnector::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &ProxyConnector::onError);
    m_socket->connectToHost(m_endpoints[0].ip, m_endpoints[0].port);
}

void ProxyConnector::onConnected()
{
    if (m_hops.isEmpty()) {
        hopDone();
        return;
    }
    emit progress(QCoreApplication::translate("ProxyConnector", "Connected to proxy %1").arg(proxyName(0)));
    m_hop = 0;
    beginHop();
}

void ProxyConnector::beginHop()
{
    m_buf.clear();
    switch (m_hops[m_hop].type) {
    case ProxyType::Socks5:
        sendSocks5Greeting();
        break;
    case ProxyType::Socks4:
        sendSocks4Connect();
        break;
    case ProxyType::Https:
    case ProxyType::Http:
        sendHttpConnect();
        break;
    }
}

void ProxyConnector::onError()
{
    if (m_finished || !m_socket)
        return;
    const QString err = m_socket->errorString();
    if (m_stage == Stage::Connecting) {
        if (m_hops.isEmpty())
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect to %1 - %2").arg(m_endpoints[0].toString(), err));
        else
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect to proxy %1 - %2").arg(proxyName(0), err));
    } else {
        fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - %2").arg(proxyName(m_hop), err));
    }
}

void ProxyConnector::fail(const QString &error)
{
    if (m_finished)
        return;
    m_finished = true;
    m_timer.stop();
    if (m_socket)
        m_socket->disconnect(this);
    emit failed(error);
}

void ProxyConnector::hopDone()
{
    if (m_hop + 1 < m_hops.size()) {
        ++m_hop;
        emit progress(QCoreApplication::translate("ProxyConnector", "Connected to proxy %1 through the chain").arg(proxyName(m_hop)));
        const QByteArray rest = m_buf;
        beginHop();
        if (!rest.isEmpty()) {
            m_buf = rest;
            onReadyRead();
        }
        return;
    }
    m_finished = true;
    m_stage = Stage::Done;
    m_timer.stop();
    m_socket->disconnect(this);
    emit connected(m_buf);
}

Endpoint ProxyConnector::nextEndpoint() const
{
    return m_endpoints[m_hop + 1];
}

void ProxyConnector::onReadyRead()
{
    if (m_finished || !m_socket)
        return;
    m_buf += m_socket->readAll();
    if (m_hops.isEmpty())
        return;
    bool ok = true;
    // Each handler consumes what it can and returns false on a fatal error.
    while (!m_finished && ok) {
        const qsizetype before = m_buf.size();
        const Stage stageBefore = m_stage;
        const int hopBefore = m_hop;
        switch (m_hops[m_hop].type) {
        case ProxyType::Socks5: ok = handleSocks5(); break;
        case ProxyType::Socks4: ok = handleSocks4(); break;
        default: ok = handleHttp(); break;
        }
        if (m_buf.size() == before && m_stage == stageBefore && m_hop == hopBefore)
            break;
    }
}

// ---- SOCKS5 ------------------------------------------------------------------

void ProxyConnector::sendSocks5Greeting()
{
    m_stage = Stage::Greeting;
    const ProxyServer &p = m_hops[m_hop];
    QByteArray g;
    g.append('\x05');
    if (p.authEnabled) {
        g.append('\x02');
        g.append('\x00');
        g.append('\x02');
    } else {
        g.append('\x01');
        g.append('\x00');
    }
    m_socket->write(g);
}

void ProxyConnector::sendSocks5Auth()
{
    m_stage = Stage::Auth;
    const ProxyServer &p = m_hops[m_hop];
    const QByteArray u = p.username.toUtf8().left(255), pw = p.password.toUtf8().left(255);
    QByteArray a;
    a.append('\x01');
    a.append(char(u.size()));
    a.append(u);
    a.append(char(pw.size()));
    a.append(pw);
    m_socket->write(a);
}

void ProxyConnector::sendSocks5Connect()
{
    m_stage = Stage::Request;
    const ProxyServer &p = m_hops[m_hop];
    const Endpoint e = nextEndpoint();
    QByteArray r("\x05\x01\x00", 3);
    const bool useName = !e.host.isEmpty() && (e.ip.isNull() || p.useHostname);
    if (useName) {
        const QByteArray h = QUrl::toAce(e.host).left(255);
        r.append('\x03');
        r.append(char(h.size()));
        r.append(h);
    } else {
        const QHostAddress ip = util::normalize(e.ip);
        if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
            r.append('\x01');
            const quint32 v = qToBigEndian(ip.toIPv4Address());
            r.append(reinterpret_cast<const char *>(&v), 4);
        } else {
            r.append('\x04');
            const Q_IPV6ADDR v = ip.toIPv6Address();
            r.append(reinterpret_cast<const char *>(&v), 16);
        }
    }
    const quint16 port = qToBigEndian(e.port);
    r.append(reinterpret_cast<const char *>(&port), 2);
    m_socket->write(r);
}

static QString socks5Error(int rep)
{
    switch (rep) {
    case 1: return QCoreApplication::translate("ProxyConnector", "general SOCKS server failure");
    case 2: return QCoreApplication::translate("ProxyConnector", "connection not allowed by ruleset");
    case 3: return QCoreApplication::translate("ProxyConnector", "network unreachable");
    case 4: return QCoreApplication::translate("ProxyConnector", "host unreachable");
    case 5: return QCoreApplication::translate("ProxyConnector", "connection refused by the target");
    case 6: return QCoreApplication::translate("ProxyConnector", "TTL expired");
    case 7: return QCoreApplication::translate("ProxyConnector", "command not supported");
    case 8: return QCoreApplication::translate("ProxyConnector", "address type not supported");
    default: return QCoreApplication::translate("ProxyConnector", "error code %1").arg(rep);
    }
}

bool ProxyConnector::handleSocks5()
{
    const QString name = proxyName(m_hop);
    switch (m_stage) {
    case Stage::Greeting: {
        if (m_buf.size() < 2)
            return true;
        const quint8 ver = quint8(m_buf[0]), method = quint8(m_buf[1]);
        m_buf.remove(0, 2);
        if (ver != 5) {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - not a SOCKS5 proxy").arg(name));
            return false;
        }
        if (method == 0x00) {
            sendSocks5Connect();
        } else if (method == 0x02 && m_hops[m_hop].authEnabled) {
            sendSocks5Auth();
        } else if (method == 0xFF) {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - proxy requires authentication").arg(name));
            return false;
        } else {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - unsupported authentication method %2").arg(name).arg(method));
            return false;
        }
        return true;
    }
    case Stage::Auth: {
        if (m_buf.size() < 2)
            return true;
        const quint8 status = quint8(m_buf[1]);
        m_buf.remove(0, 2);
        if (status != 0) {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - authentication failed (wrong username or password)").arg(name));
            return false;
        }
        sendSocks5Connect();
        return true;
    }
    case Stage::Request: {
        if (m_buf.size() < 5)
            return true;
        const quint8 rep = quint8(m_buf[1]), atyp = quint8(m_buf[3]);
        int len;
        if (atyp == 1)
            len = 4 + 4 + 2;
        else if (atyp == 4)
            len = 4 + 16 + 2;
        else if (atyp == 3)
            len = 4 + 1 + quint8(m_buf[4]) + 2;
        else {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - malformed reply").arg(name));
            return false;
        }
        if (rep != 0) {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - %2").arg(name, socks5Error(rep)));
            return false;
        }
        if (m_buf.size() < len)
            return true;
        m_buf.remove(0, len);
        hopDone();
        return true;
    }
    default:
        return true;
    }
}

// ---- SOCKS4 / 4A -----------------------------------------------------------

void ProxyConnector::sendSocks4Connect()
{
    m_stage = Stage::Request;
    const ProxyServer &p = m_hops[m_hop];
    const Endpoint e = nextEndpoint();
    QByteArray r("\x04\x01", 2);
    const quint16 port = qToBigEndian(e.port);
    r.append(reinterpret_cast<const char *>(&port), 2);
    const QHostAddress ip = util::normalize(e.ip);
    const bool useName = p.socks4a && !e.host.isEmpty() && (ip.isNull() || p.useHostname);
    if (!useName && ip.protocol() != QAbstractSocket::IPv4Protocol) {
        fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - SOCKS4 does not support IPv6 targets").arg(proxyName(m_hop)));
        return;
    }
    if (useName) {
        r.append("\x00\x00\x00\x01", 4);
    } else {
        const quint32 v = qToBigEndian(ip.toIPv4Address());
        r.append(reinterpret_cast<const char *>(&v), 4);
    }
    r.append(p.username.toUtf8());
    r.append('\0');
    if (useName) {
        r.append(QUrl::toAce(e.host));
        r.append('\0');
    }
    m_socket->write(r);
}

bool ProxyConnector::handleSocks4()
{
    if (m_buf.size() < 8)
        return true;
    const quint8 status = quint8(m_buf[1]);
    m_buf.remove(0, 8);
    if (status != 0x5A) {
        QString why = status == 0x5C || status == 0x5D ? QCoreApplication::translate("ProxyConnector", "identd authentication failed") : QCoreApplication::translate("ProxyConnector", "request rejected or failed");
        fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - %2 (code %3)").arg(proxyName(m_hop), why).arg(status));
        return false;
    }
    hopDone();
    return true;
}

// ---- HTTPS CONNECT ---------------------------------------------------------

void ProxyConnector::sendHttpConnect()
{
    m_stage = Stage::Request;
    const ProxyServer &p = m_hops[m_hop];
    const Endpoint e = nextEndpoint();
    QString authority;
    const bool useName = !e.host.isEmpty() && (e.ip.isNull() || p.useHostname);
    if (useName)
        authority = QString("%1:%2").arg(QString::fromLatin1(QUrl::toAce(e.host))).arg(e.port);
    else
        authority = util::formatEndpoint(e.ip, e.port);
    QByteArray req = "CONNECT " + authority.toLatin1() + " HTTP/1.1\r\nHost: " + authority.toLatin1() + "\r\n";
    if (p.authEnabled) {
        const QByteArray cred = (p.username + ":" + p.password).toUtf8().toBase64();
        req += "Proxy-Authorization: Basic " + cred + "\r\n";
    }
    req += "Proxy-Connection: keep-alive\r\n\r\n";
    m_socket->write(req);
}

bool ProxyConnector::handleHttp()
{
    const qsizetype end = m_buf.indexOf("\r\n\r\n");
    if (end < 0) {
        if (m_buf.size() > 16384) {
            fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - response header too large").arg(proxyName(m_hop)));
            return false;
        }
        return true;
    }
    const QByteArray head = m_buf.left(end);
    m_buf.remove(0, end + 4);
    const QList<QByteArray> parts = head.left(head.indexOf("\r\n") < 0 ? head.size() : head.indexOf("\r\n")).split(' ');
    const int code = parts.size() >= 2 ? parts[1].toInt() : 0;
    if (code == 200) {
        hopDone();
        return true;
    }
    QString why;
    if (code == 407)
        why = QCoreApplication::translate("ProxyConnector", "proxy requires authentication (HTTP 407)");
    else if (code == 0)
        why = QCoreApplication::translate("ProxyConnector", "not an HTTP proxy");
    else
        why = QCoreApplication::translate("ProxyConnector", "proxy returned HTTP %1").arg(QString::fromLatin1(parts.mid(1).join(' ')));
    fail(QCoreApplication::translate("ProxyConnector", "Could not connect through proxy %1 - %2").arg(proxyName(m_hop), why));
    return false;
}
