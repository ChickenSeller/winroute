#include "UdpRelaySession.h"
#include "DnsClient.h"
#include "Logger.h"
#include "NetFilterApi.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QUrl>
#include <QtEndian>

static constexpr int kConnectTimeoutMs = 20000;
static constexpr int kIdleTimeoutMs = 120000;

namespace {

// sockaddr bytes (as the driver passes them) -> address and port.
QHostAddress sockaddrToHost(const QByteArray &sa, quint16 *port)
{
    *port = 0;
    if (sa.size() < int(sizeof(sockaddr)))
        return {};
    const auto *s = reinterpret_cast<const sockaddr *>(sa.constData());
    if (s->sa_family == AF_INET && sa.size() >= int(sizeof(sockaddr_in))) {
        const auto *in = reinterpret_cast<const sockaddr_in *>(sa.constData());
        *port = ntohs(in->sin_port);
        return QHostAddress(ntohl(in->sin_addr.s_addr));
    }
    if (s->sa_family == AF_INET6 && sa.size() >= int(sizeof(sockaddr_in6))) {
        const auto *in6 = reinterpret_cast<const sockaddr_in6 *>(sa.constData());
        *port = ntohs(in6->sin6_port);
        return util::normalize(QHostAddress(reinterpret_cast<const quint8 *>(&in6->sin6_addr)));
    }
    return {};
}

void appendAddress(QByteArray *out, const QHostAddress &ip)
{
    const QHostAddress a = util::normalize(ip);
    if (a.protocol() == QAbstractSocket::IPv6Protocol) {
        out->append('\x04');
        const Q_IPV6ADDR v = a.toIPv6Address();
        out->append(reinterpret_cast<const char *>(&v), 16);
    } else {
        out->append('\x01');
        const quint32 v = qToBigEndian(a.toIPv4Address());
        out->append(reinterpret_cast<const char *>(&v), 4);
    }
}

} // namespace

QString UdpRelaySession::targetText(const QByteArray &remote, const QString &hostname)
{
    quint16 p = 0;
    const QHostAddress a = sockaddrToHost(remote, &p);
    return hostname.isEmpty() ? util::formatEndpoint(a, p) : QString("%1:%2").arg(hostname).arg(p);
}

UdpRelaySession::UdpRelaySession(Relay *relay, const UdpDatagram &first, const ProxyServer &proxy, QObject *parent)
    : QObject(parent), m_relay(relay), m_api(NetFilterApi::load(nullptr)), m_id(first.id), m_proxy(proxy),
      m_appName(first.appName)
{
    quint16 p = 0;
    const QHostAddress a = sockaddrToHost(first.remote, &p);
    m_targetText = first.hostname.isEmpty() ? util::formatEndpoint(a, p) : QString("%1:%2").arg(first.hostname).arg(p);
    m_idle.setSingleShot(true);
    m_idle.setInterval(kIdleTimeoutMs);
    connect(&m_idle, &QTimer::timeout, this, &UdpRelaySession::finish);
    m_idle.start();
    m_pending << first;
    // Deferred so tests can inject sinks between construction and start.
    QTimer::singleShot(0, this, [this] { start(); });
}

bool UdpRelaySession::trafficChanged()
{
    const bool changed = m_sent != m_lastSent || m_received != m_lastReceived;
    m_lastSent = m_sent;
    m_lastReceived = m_received;
    return changed;
}

void UdpRelaySession::start()
{
    if (m_finished)
        return;
    if (!m_deliverSink && (!m_api || !m_api->udpPostReceive || !m_api->udpPostSend)) {
        toBypass(QCoreApplication::translate("Relay", "the network driver API is unavailable"));
        return;
    }

    const QHostAddress literal(m_proxy.address);
    if (!literal.isNull()) {
        m_stage = Stage::Connecting;
        openControl(literal);
        return;
    }
    DnsClient *dns = m_relay->dns();
    if (!dns) {
        toBypass(QCoreApplication::translate("Relay", "no resolver available"));
        return;
    }
    QPointer<UdpRelaySession> self(this);
    const QString host = m_proxy.address;
    dns->resolve(host, [self, host](const QList<QHostAddress> &addrs, const QString &err) {
        if (!self || self->m_finished)
            return;
        if (addrs.isEmpty()) {
            self->toBypass(QCoreApplication::translate("Relay", "could not resolve proxy %1 - %2").arg(host, err));
            return;
        }
        self->m_stage = Stage::Connecting;
        self->openControl(addrs.first());
    });
}

void UdpRelaySession::openControl(const QHostAddress &proxyIp)
{
    m_control = new QTcpSocket(this);
    m_control->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(m_control, &QTcpSocket::connected, this, &UdpRelaySession::sendGreeting);
    connect(m_control, &QTcpSocket::readyRead, this, &UdpRelaySession::onControlReadyRead);
    connect(m_control, &QTcpSocket::disconnected, this, &UdpRelaySession::onControlClosed);
    connect(m_control, &QTcpSocket::errorOccurred, this, &UdpRelaySession::onControlClosed);
    m_control->connectToHost(proxyIp, m_proxy.port);
}

void UdpRelaySession::sendGreeting()
{
    m_stage = Stage::Greeting;
    QByteArray g;
    g.append('\x05');
    if (m_proxy.authEnabled) {
        g.append('\x02');
        g.append('\x00');
        g.append('\x02');
    } else {
        g.append('\x01');
        g.append('\x00');
    }
    m_control->write(g);
}

void UdpRelaySession::sendAuth()
{
    m_stage = Stage::Auth;
    const QByteArray u = m_proxy.username.toUtf8().left(255), pw = m_proxy.password.toUtf8().left(255);
    QByteArray a;
    a.append('\x01');
    a.append(char(u.size()));
    a.append(u);
    a.append(char(pw.size()));
    a.append(pw);
    m_control->write(a);
}

void UdpRelaySession::sendAssociate()
{
    m_stage = Stage::Associate;
    // CMD = 3 (UDP ASSOCIATE), DST = 0.0.0.0:0 (we do not announce our address).
    QByteArray r("\x05\x03\x00\x01\x00\x00\x00\x00\x00\x00", 10);
    m_control->write(r);
}

void UdpRelaySession::onControlReadyRead()
{
    if (m_finished || !m_control)
        return;
    m_ctlBuf += m_control->readAll();
    bool progress = true;
    while (progress && !m_finished) {
        progress = false;
        if (m_stage == Stage::Greeting) {
            if (m_ctlBuf.size() < 2)
                return;
            const quint8 ver = quint8(m_ctlBuf[0]), method = quint8(m_ctlBuf[1]);
            m_ctlBuf.remove(0, 2);
            if (ver != 5) {
                toBypass(QCoreApplication::translate("Relay", "not a SOCKS5 proxy"));
                return;
            }
            if (method == 0x00)
                sendAssociate();
            else if (method == 0x02 && m_proxy.authEnabled)
                sendAuth();
            else {
                toBypass(QCoreApplication::translate("Relay", "SOCKS5 authentication method not accepted"));
                return;
            }
            progress = true;
        } else if (m_stage == Stage::Auth) {
            if (m_ctlBuf.size() < 2)
                return;
            const quint8 status = quint8(m_ctlBuf[1]);
            m_ctlBuf.remove(0, 2);
            if (status != 0) {
                toBypass(QCoreApplication::translate("Relay", "SOCKS5 authentication failed"));
                return;
            }
            sendAssociate();
            progress = true;
        } else if (m_stage == Stage::Associate) {
            if (!parseAssociateReply())
                return;
            progress = true;
        } else {
            // After the association the proxy keeps the control connection open
            // but sends nothing; ignore anything unexpected.
            m_ctlBuf.clear();
            return;
        }
    }
}

bool UdpRelaySession::parseAssociateReply()
{
    if (m_ctlBuf.size() < 5)
        return false;
    const quint8 rep = quint8(m_ctlBuf[1]), atyp = quint8(m_ctlBuf[3]);
    int len;
    if (atyp == 1)
        len = 4 + 4 + 2;
    else if (atyp == 4)
        len = 4 + 16 + 2;
    else if (atyp == 3)
        len = 4 + 1 + quint8(m_ctlBuf[4]) + 2;
    else {
        toBypass(QCoreApplication::translate("Relay", "malformed SOCKS5 reply"));
        return false;
    }
    if (rep != 0) {
        toBypass(QCoreApplication::translate("Relay", "the proxy does not support UDP (SOCKS5 reply %1)").arg(rep));
        return false;
    }
    if (m_ctlBuf.size() < len)
        return false;

    QHostAddress relayAddr;
    quint16 relayPort = 0;
    const char *p = m_ctlBuf.constData() + 4;
    if (atyp == 1) {
        quint32 v;
        memcpy(&v, p, 4);
        relayAddr = QHostAddress(qFromBigEndian(v));
        memcpy(&relayPort, p + 4, 2);
    } else if (atyp == 4) {
        relayAddr = util::normalize(QHostAddress(reinterpret_cast<const quint8 *>(p)));
        memcpy(&relayPort, p + 16, 2);
    } else {
        // A domain name relay endpoint is unusual; fall back to the proxy IP.
        memcpy(&relayPort, p + 1 + quint8(m_ctlBuf[4]), 2);
    }
    relayPort = qFromBigEndian(relayPort);
    m_ctlBuf.remove(0, len);

    // A wildcard or missing relay address means "same host as the control
    // connection", i.e. the proxy itself.
    if (relayAddr.isNull() || relayAddr.isEqual(QHostAddress::AnyIPv4) || relayAddr.isEqual(QHostAddress::AnyIPv6))
        relayAddr = util::normalize(m_control->peerAddress());
    openUdp(relayAddr, relayPort);
    return true;
}

void UdpRelaySession::openUdp(const QHostAddress &relayAddr, quint16 relayPort)
{
    m_relayAddr = relayAddr;
    m_relayPort = relayPort;
    m_udp = new QUdpSocket(this);
    const bool v6 = relayAddr.protocol() == QAbstractSocket::IPv6Protocol;
    if (!m_udp->bind(v6 ? QHostAddress(QHostAddress::AnyIPv6) : QHostAddress(QHostAddress::AnyIPv4), 0)) {
        toBypass(QCoreApplication::translate("Relay", "cannot open the local UDP socket"));
        return;
    }
    connect(m_udp, &QUdpSocket::readyRead, this, &UdpRelaySession::onUdpReadyRead);
    m_stage = Stage::Ready;
    logNormal(QCoreApplication::translate("Relay", "%1 - %2 open through proxy %3:%4 SOCKS5 (UDP)")
                  .arg(m_appName, m_targetText, m_proxy.address)
                  .arg(m_proxy.port));
    emit m_relay->statusChanged(m_id, "Open");
    flushPending();
}

void UdpRelaySession::flushPending()
{
    const QList<UdpDatagram> pending = m_pending;
    m_pending.clear();
    for (const UdpDatagram &dg : pending) {
        if (m_stage == Stage::Ready)
            sendThrough(dg);
        else
            bypass(dg);
    }
}

void UdpRelaySession::send(const UdpDatagram &dg)
{
    if (m_finished)
        return;
    touch();
    switch (m_stage) {
    case Stage::Ready:
        sendThrough(dg);
        break;
    case Stage::Bypass:
        bypass(dg);
        break;
    case Stage::Closed:
        break;
    default:
        m_pending << dg;    // still associating
        break;
    }
}

void UdpRelaySession::sendThrough(const UdpDatagram &dg)
{
    quint16 port = 0;
    const QHostAddress target = sockaddrToHost(dg.remote, &port);

    QByteArray pkt;
    pkt.append('\0');
    pkt.append('\0');   // RSV
    pkt.append('\0');   // FRAG
    if (dg.fakeIp && !dg.hostname.isEmpty()) {
        const QByteArray h = QUrl::toAce(dg.hostname).left(255);
        pkt.append('\x03');
        pkt.append(char(h.size()));
        pkt.append(h);
    } else {
        appendAddress(&pkt, target);
    }
    const quint16 be = qToBigEndian(port);
    pkt.append(reinterpret_cast<const char *>(&be), 2);
    pkt.append(dg.data);

    m_lastRemote = dg.remote;
    m_lastOpts = dg.opts;
    // Map the real server address back to what the application sent to, so the
    // reply is delivered as from that address. For fake-DNS domain targets the
    // real IP is only learned from the reply, so we also keep it as the fallback.
    if (!dg.fakeIp && !target.isNull()) {
        quint16 rp = port;
        m_remoteByOrigin.insert(util::formatEndpoint(target, rp), dg.remote);
    }

    m_udp->writeDatagram(pkt, m_relayAddr, m_relayPort);
    m_sent += dg.data.size();
}

void UdpRelaySession::bypass(const UdpDatagram &dg)
{
    if (m_bypassSink) {
        m_bypassSink(dg);
        m_sent += dg.data.size();
        return;
    }
    if (!m_api || !m_api->udpPostSend)
        return;
    auto *opt = const_cast<PNF_UDP_OPTIONS>(reinterpret_cast<const NF_UDP_OPTIONS *>(dg.opts.constData()));
    m_api->udpPostSend(dg.id, reinterpret_cast<const unsigned char *>(dg.remote.constData()),
                       dg.data.constData(), dg.data.size(), dg.opts.isEmpty() ? nullptr : opt);
    m_sent += dg.data.size();
}

void UdpRelaySession::onUdpReadyRead()
{
    if (m_finished || !m_udp)
        return;
    touch();
    while (m_udp->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(int(m_udp->pendingDatagramSize()));
        QHostAddress from;
        quint16 fromPort = 0;
        const qint64 n = m_udp->readDatagram(buf.data(), buf.size(), &from, &fromPort);
        if (n < 0)
            break;
        buf.resize(int(n));
        if (buf.size() < 4)
            continue;
        if (quint8(buf[2]) != 0)
            continue;   // a fragment; not supported
        const quint8 atyp = quint8(buf[3]);
        int hdr;
        QHostAddress origin;
        quint16 oport = 0;
        if (atyp == 1) {
            if (buf.size() < 10)
                continue;
            quint32 v;
            memcpy(&v, buf.constData() + 4, 4);
            origin = QHostAddress(qFromBigEndian(v));
            memcpy(&oport, buf.constData() + 8, 2);
            hdr = 10;
        } else if (atyp == 4) {
            if (buf.size() < 22)
                continue;
            origin = util::normalize(QHostAddress(reinterpret_cast<const quint8 *>(buf.constData() + 4)));
            memcpy(&oport, buf.constData() + 20, 2);
            hdr = 22;
        } else if (atyp == 3) {
            const int dlen = quint8(buf[4]);
            if (buf.size() < 4 + 1 + dlen + 2)
                continue;
            memcpy(&oport, buf.constData() + 4 + 1 + dlen, 2);
            hdr = 4 + 1 + dlen + 2;
        } else {
            continue;
        }
        oport = qFromBigEndian(oport);
        deliver(origin, oport, buf.mid(hdr));
    }
}

void UdpRelaySession::deliver(const QHostAddress &origin, quint16 oport, const QByteArray &payload)
{
    // The source to show the application: the address it sent to. Match the real
    // origin if we know it, otherwise use the most recent target.
    QByteArray remote = m_remoteByOrigin.value(util::formatEndpoint(origin, oport));
    if (remote.isEmpty())
        remote = m_lastRemote;
    if (remote.isEmpty())
        return;
    if (m_deliverSink) {
        m_deliverSink(remote, payload);
        m_received += payload.size();
        return;
    }
    if (!m_api || !m_api->udpPostReceive || m_lastOpts.isEmpty())
        return;
    auto *opt = const_cast<PNF_UDP_OPTIONS>(reinterpret_cast<const NF_UDP_OPTIONS *>(m_lastOpts.constData()));
    m_api->udpPostReceive(m_id, reinterpret_cast<const unsigned char *>(remote.constData()),
                          payload.constData(), payload.size(), opt);
    m_received += payload.size();
}

void UdpRelaySession::toBypass(const QString &reason)
{
    if (m_stage == Stage::Bypass || m_finished)
        return;
    m_stage = Stage::Bypass;
    if (!m_warnedBypass) {
        m_warnedBypass = true;
        logNormal(QCoreApplication::translate("Relay", "%1 - %2 UDP cannot use the proxy (%3); sending directly")
                      .arg(m_appName, m_targetText, reason));
    }
    if (m_control)
        m_control->disconnect(this);
    flushPending();
}

void UdpRelaySession::onControlClosed()
{
    if (m_finished)
        return;
    if (m_stage == Stage::Ready) {
        // The association ended; nothing more can be tunnelled.
        finish();
    } else if (m_stage != Stage::Bypass) {
        toBypass(QCoreApplication::translate("Relay", "the proxy closed the control connection"));
    }
}

void UdpRelaySession::touch()
{
    if (!m_finished)
        m_idle.start();
}

void UdpRelaySession::close()
{
    finish();
}

void UdpRelaySession::finish()
{
    if (m_finished)
        return;
    m_finished = true;
    m_idle.stop();
    if (m_control)
        m_control->abort();
    if (m_udp)
        m_udp->close();
    if (m_stage == Stage::Ready || m_sent > 0 || m_received > 0)
        logVerbose(QCoreApplication::translate("Relay", "%1 - %2 UDP closed, %3 sent, %4 received")
                       .arg(m_appName, m_targetText)
                       .arg(m_sent)
                       .arg(m_received));
    m_relay->udpSessionFinished(m_id, m_sent, m_received);
    deleteLater();
}
