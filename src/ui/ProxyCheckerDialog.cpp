#include "ProxyCheckerDialog.h"
#include "ui_ProxyCheckerDialog.h"

#include "core/DnsClient.h"
#include "core/ProxyConnector.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QSslError>
#include <QSslSocket>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTimer>
#include <QUdpSocket>
#include <QVBoxLayout>
#include <QtEndian>

static const char *kOk = "<span style='color:#1a8a2a;font-weight:bold'>OK</span>";
static const char *kFailed = "<span style='color:#c0201a;font-weight:bold'>FAILED</span>";

ProxyCheckerDialog::ProxyCheckerDialog(const QList<ProxyServer> &proxies, QWidget *parent)
    : QDialog(parent)
{
    m_choices << qMakePair(proxies.size() == 1 ? proxies[0].displayName() : tr("Chain"), proxies);
    build();
}

ProxyCheckerDialog::ProxyCheckerDialog(const QList<QPair<QString, QList<ProxyServer>>> &choices, QWidget *parent)
    : QDialog(parent), m_choices(choices)
{
    build();
}

void ProxyCheckerDialog::build()
{
    ui = std::make_unique<Ui::ProxyCheckerDialog>();
    ui->setupUi(this);
    // Keep the existing member pointers as aliases of the .ui widgets, so the
    // test logic below is unchanged.
    m_choice = ui->choiceCombo;
    m_host = ui->hostEdit;
    m_port = ui->portSpin;
    m_output = ui->outputEdit;
    m_start = ui->startButton;

    m_dns = new DnsClient(this);
    m_dns->bind(0);

    for (const auto &c : m_choices)
        m_choice->addItem(c.first);

    connect(m_start, &QPushButton::clicked, this, &ProxyCheckerDialog::startTest);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

ProxyCheckerDialog::~ProxyCheckerDialog() = default;

void ProxyCheckerDialog::out(const QString &html)
{
    m_output->append(html);
}

void ProxyCheckerDialog::startTest()
{
    if (m_running || m_choices.isEmpty())
        return;
    m_running = true;
    m_start->setEnabled(false);
    m_output->clear();
    m_hops = m_choices[qMax(0, m_choice->currentIndex())].second;
    out(tr("<b>Testing Started...</b>"));
    for (int i = 0; i < m_hops.size(); ++i) {
        const ProxyServer &p = m_hops[i];
        out(tr("Proxy Server%1<br>&nbsp;&nbsp;Address: %2:%3<br>&nbsp;&nbsp;Protocol: %4<br>&nbsp;&nbsp;Authentication: %5")
                .arg(m_hops.size() > 1 ? QString(" #%1").arg(i + 1) : QString())
                .arg(p.address.toHtmlEscaped())
                .arg(p.port)
                .arg(proxyTypeToString(p.type))
                .arg(p.authEnabled ? tr("YES") : tr("NO")));
    }
    out("");
    step1();
}

// Step 1: plain TCP connection to the first proxy.
void ProxyCheckerDialog::step1()
{
    out(tr("<b>Connection Test</b>"));
    out(tr("Connecting to %1:%2 ...").arg(m_hops[0].address.toHtmlEscaped()).arg(m_hops[0].port));
    m_socket = new QTcpSocket(this);
    m_timer.start();
    m_connector = new ProxyConnector(m_socket, {}, Endpoint{QHostAddress(m_hops[0].address).isNull() ? m_hops[0].address : QString(),
                                                            QHostAddress(m_hops[0].address), m_hops[0].port},
                                     m_dns, 10000, this);
    connect(m_connector, &ProxyConnector::connected, this, [this] {
        out(tr("Connection established %1 (%2 ms)").arg(kOk).arg(m_timer.elapsed()));
        m_connector->deleteLater();
        m_socket->abort();
        m_socket->deleteLater();
        out("");
        step2();
    });
    connect(m_connector, &ProxyConnector::failed, this, [this](const QString &err) {
        out(QString("%1 : %2").arg(kFailed, err.toHtmlEscaped()));
        finish(false);
    });
    m_connector->start();
}

// Step 2: tunnel to the target through the proxy (or chain). For an HTTPS port a
// real TLS handshake runs over the tunnel before the HTTP request; otherwise the
// request is sent in the clear.
void ProxyCheckerDialog::step2()
{
    const QString host = m_host->text().trimmed();
    const quint16 port = quint16(m_port->value());
    out(tr("<b>Connection through the Proxy Server Test</b>"));
    out(tr("Connecting to %1:%2 through the proxy ...").arg(host.toHtmlEscaped()).arg(port));
    m_socket = new QSslSocket(this);
    m_timer.start();
    Endpoint target;
    target.host = host;
    target.ip = QHostAddress(host);
    target.port = port;
    m_connector = new ProxyConnector(m_socket, m_hops, target, m_dns, 15000, this);
    connect(m_connector, &ProxyConnector::progress, this, [this](const QString &msg) { out(msg.toHtmlEscaped()); });
    connect(m_connector, &ProxyConnector::failed, this, [this](const QString &err) {
        out(QString("%1 : %2").arg(kFailed, err.toHtmlEscaped()));
        finish(false);
    });
    connect(m_connector, &ProxyConnector::connected, this, [this, host, port](const QByteArray &leftover) {
        out(tr("Connection to %1 established through the proxy %2 (%3 ms)").arg(host.toHtmlEscaped(), kOk).arg(m_timer.elapsed()));
        out("");
        if (port != 443) {
            httpRequest(host, leftover);
            return;
        }
        if (!QSslSocket::supportsSsl()) {
            out(tr("TLS backend unavailable; skipping the HTTPS request test (the tunnel is up)."));
            afterTcpTests();
            return;
        }
        // HTTPS: a plaintext GET to a TLS port gets no answer, so do a real TLS
        // handshake over the tunnel first, then the request over it.
        out(tr("<b>TLS Handshake Test</b>"));
        auto *ssl = qobject_cast<QSslSocket *>(m_socket.data());
        m_timer.start();
        connect(ssl, &QSslSocket::encrypted, this, [this, host] {
            out(tr("TLS handshake %1 (%2 ms)").arg(kOk).arg(m_timer.elapsed()));
            out("");
            httpRequest(host, QByteArray());
        });
        connect(ssl, &QSslSocket::sslErrors, this, [this, ssl](const QList<QSslError> &errs) {
            // A certificate problem still proves the tunnel carries a real TLS
            // session end to end, which is what we check here; note it and go on.
            QStringList names;
            for (const QSslError &e : errs)
                names << e.errorString();
            out(tr("TLS warning: %1").arg(names.join("; ").toHtmlEscaped()));
            ssl->ignoreSslErrors();
        });
        connect(ssl, &QSslSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            if (m_running && m_socket) {
                out(QString("%1 : %2").arg(kFailed, m_socket->errorString().toHtmlEscaped()));
                finish(false);
            }
        });
        ssl->setPeerVerifyName(host);
        ssl->startClientEncryption();
        QTimer::singleShot(10000, this, [this, ssl] {
            if (m_running && ssl && !ssl->isEncrypted()) {
                out(tr("%1 : TLS handshake timed out").arg(kFailed));
                finish(false);
            }
        });
    });
    m_connector->start();
}

// Sends GET / over the (possibly already TLS-encrypted) socket and reports the
// status line. On success it moves on to the UDP test.
void ProxyCheckerDialog::httpRequest(const QString &host, const QByteArray &leftover)
{
    out(tr("<b>HTTP Request Test</b>"));
    m_httpBuf = leftover;
    m_httpDone = false;
    connect(m_socket, &QTcpSocket::readyRead, this, [this] {
        m_httpBuf += m_socket->readAll();
        const qsizetype eol = m_httpBuf.indexOf("\r\n");
        if (eol < 0)
            return;
        out(tr("Response: %1 %2").arg(QString::fromLatin1(m_httpBuf.left(eol)).toHtmlEscaped(), kOk));
        m_httpDone = true;
        m_socket->disconnect(this);
        afterTcpTests();
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this] {
        if (!m_httpDone) {
            out(tr("%1 : connection closed without a response").arg(kFailed));
            finish(false);
        }
    });
    m_socket->write(QString("GET / HTTP/1.1\r\nHost: %1\r\nUser-Agent: WinRoute ProxyChecker\r\nConnection: close\r\n\r\n")
                        .arg(host)
                        .toLatin1());
    QTimer::singleShot(10000, this, [this] {
        if (m_running && !m_httpDone) {
            out(tr("%1 : no response within 10 seconds").arg(kFailed));
            finish(false);
        }
    });
}

// After the TCP tests pass, test UDP for a single SOCKS5 proxy (UDP ASSOCIATE
// applies to SOCKS5 only, and WinRoute tunnels UDP only through a single hop).
void ProxyCheckerDialog::afterTcpTests()
{
    out("");
    if (m_hops.size() == 1 && m_hops[0].type == ProxyType::Socks5) {
        udpTest();
        return;
    }
    if (m_hops.size() > 1)
        out(tr("UDP test skipped: a proxy chain cannot be tested for UDP."));
    else
        out(tr("UDP test skipped: UDP applies to SOCKS5 proxies only."));
    finish(true);
}

// ---- UDP support (SOCKS5 UDP ASSOCIATE) -------------------------------------

void ProxyCheckerDialog::udpTest()
{
    const ProxyServer &p = m_hops[0];
    out(tr("<b>UDP Support Test (SOCKS5 UDP ASSOCIATE)</b>"));
    out(tr("Requesting a UDP relay from %1:%2 ...").arg(p.address.toHtmlEscaped()).arg(p.port));
    m_udpBuf.clear();
    m_udpStage = 0;
    m_udpPending = true;
    m_udpCtrl = new QTcpSocket(this);
    connect(m_udpCtrl, &QTcpSocket::connected, this, [this] {
        const ProxyServer &p = m_hops[0];
        QByteArray g("\x05", 1);
        if (p.authEnabled) {
            g.append('\x02');
            g.append('\x00');
            g.append('\x02');
        } else {
            g.append('\x01');
            g.append('\x00');
        }
        m_udpStage = 0;
        m_udpCtrl->write(g);
    });
    connect(m_udpCtrl, &QTcpSocket::readyRead, this, &ProxyCheckerDialog::udpCtrlReadyRead);
    connect(m_udpCtrl, &QTcpSocket::errorOccurred, this, [this] {
        udpDone(false, m_udpCtrl ? m_udpCtrl->errorString() : QString());
    });
    m_udpCtrl->connectToHost(p.address, p.port);
    QTimer::singleShot(12000, this, [this] {
        udpDone(false, tr("the proxy did not grant a UDP relay in time"));
    });
}

void ProxyCheckerDialog::sendUdpAssociate()
{
    m_udpStage = 2;
    // CMD=3 (UDP ASSOCIATE), ATYP=IPv4, address 0.0.0.0, port 0.
    m_udpCtrl->write(QByteArray("\x05\x03\x00\x01\x00\x00\x00\x00\x00\x00", 10));
}

void ProxyCheckerDialog::udpCtrlReadyRead()
{
    if (!m_udpCtrl || !m_udpPending)
        return;
    m_udpBuf += m_udpCtrl->readAll();
    const ProxyServer &p = m_hops[0];

    if (m_udpStage == 0) {   // greeting reply
        if (m_udpBuf.size() < 2)
            return;
        const quint8 ver = quint8(m_udpBuf[0]), method = quint8(m_udpBuf[1]);
        m_udpBuf.remove(0, 2);
        if (ver != 5) {
            udpDone(false, tr("not a SOCKS5 proxy"));
        } else if (method == 0x00) {
            sendUdpAssociate();
        } else if (method == 0x02 && p.authEnabled) {
            const QByteArray u = p.username.toUtf8().left(255), pw = p.password.toUtf8().left(255);
            QByteArray a("\x01", 1);
            a.append(char(u.size()));
            a.append(u);
            a.append(char(pw.size()));
            a.append(pw);
            m_udpStage = 1;
            m_udpCtrl->write(a);
        } else if (method == 0xFF) {
            udpDone(false, tr("proxy requires authentication"));
        } else {
            udpDone(false, tr("unsupported authentication method"));
        }
        return;
    }
    if (m_udpStage == 1) {   // auth reply
        if (m_udpBuf.size() < 2)
            return;
        const quint8 status = quint8(m_udpBuf[1]);
        m_udpBuf.remove(0, 2);
        if (status != 0)
            udpDone(false, tr("authentication failed"));
        else
            sendUdpAssociate();
        return;
    }
    if (m_udpStage == 2) {   // UDP ASSOCIATE reply: VER REP RSV ATYP BND.ADDR BND.PORT
        if (m_udpBuf.size() < 5)
            return;
        const quint8 rep = quint8(m_udpBuf[1]), atyp = quint8(m_udpBuf[3]);
        int need;
        if (atyp == 1)
            need = 4 + 4 + 2;
        else if (atyp == 4)
            need = 4 + 16 + 2;
        else if (atyp == 3)
            need = 4 + 1 + quint8(m_udpBuf[4]) + 2;
        else {
            udpDone(false, tr("malformed reply"));
            return;
        }
        if (rep != 0) {
            udpDone(false, tr("UDP ASSOCIATE rejected (code %1)").arg(rep));
            return;
        }
        if (m_udpBuf.size() < need)
            return;
        const auto *d = reinterpret_cast<const uchar *>(m_udpBuf.constData());
        if (atyp == 1) {
            m_relayAddr = QHostAddress(qFromBigEndian<quint32>(d + 4));
            m_relayPort = qFromBigEndian<quint16>(d + 8);
        } else if (atyp == 4) {
            Q_IPV6ADDR v;
            memcpy(&v, d + 4, 16);
            m_relayAddr = QHostAddress(v);
            m_relayPort = qFromBigEndian<quint16>(d + 20);
        } else {
            m_relayAddr = QHostAddress();
            m_relayPort = qFromBigEndian<quint16>(d + need - 2);
        }
        // "0.0.0.0" (or a name) means: reuse the proxy's own address.
        if (m_relayAddr.isNull() || m_relayAddr.toIPv4Address() == 0)
            m_relayAddr = m_udpCtrl->peerAddress();
        m_udpBuf.clear();
        out(tr("Relay granted at %1:%2").arg(m_relayAddr.toString()).arg(m_relayPort));
        udpProbe();
    }
}

void ProxyCheckerDialog::udpProbe()
{
    m_udp = new QUdpSocket(this);
    if (!m_udp->bind(QHostAddress(QHostAddress::AnyIPv4), 0)) {
        udpDone(false, tr("cannot open a local UDP socket"));
        return;
    }
    connect(m_udp, &QUdpSocket::readyRead, this, &ProxyCheckerDialog::onUdpDatagram);

    // A DNS A-record query for example.com.
    m_dnsId = quint16(QRandomGenerator::global()->generate());
    QByteArray dns;
    auto be16 = [&dns](quint16 v) { dns.append(char(v >> 8)); dns.append(char(v & 0xff)); };
    be16(m_dnsId);
    be16(0x0100);   // standard query, recursion desired
    be16(1);
    be16(0);
    be16(0);
    be16(0);
    for (const QByteArray &label : QByteArray("example.com").split('.')) {
        dns.append(char(label.size()));
        dns.append(label);
    }
    dns.append('\0');
    be16(1);   // QTYPE A
    be16(1);   // QCLASS IN

    // Wrap it in a SOCKS5 UDP header targeting 8.8.8.8:53.
    QByteArray pkt("\x00\x00\x00\x01", 4);   // RSV, FRAG, ATYP=IPv4
    const quint32 dst = qToBigEndian<quint32>(QHostAddress("8.8.8.8").toIPv4Address());
    pkt.append(reinterpret_cast<const char *>(&dst), 4);
    const quint16 dport = qToBigEndian<quint16>(53);
    pkt.append(reinterpret_cast<const char *>(&dport), 2);
    pkt.append(dns);

    out(tr("Sending a DNS query to 8.8.8.8:53 through the relay ..."));
    m_udp->writeDatagram(pkt, m_relayAddr, m_relayPort);
    QTimer::singleShot(5000, this, [this] {
        udpDone(false, tr("no reply from the relay within 5 seconds (UDP may be blocked)"));
    });
}

void ProxyCheckerDialog::onUdpDatagram()
{
    while (m_udp && m_udp->hasPendingDatagrams()) {
        QByteArray d(int(m_udp->pendingDatagramSize()), Qt::Uninitialized);
        m_udp->readDatagram(d.data(), d.size());
        if (d.size() < 4)
            continue;
        const quint8 atyp = quint8(d[3]);
        int off;
        if (atyp == 1)
            off = 4 + 4 + 2;
        else if (atyp == 4)
            off = 4 + 16 + 2;
        else if (atyp == 3)
            off = 4 + 1 + quint8(d[4]) + 2;
        else
            continue;
        if (d.size() < off + 4)
            continue;
        const QByteArray payload = d.mid(off);
        const quint16 id = (quint8(payload[0]) << 8) | quint8(payload[1]);
        const bool isResponse = quint8(payload[2]) & 0x80;
        if (id == m_dnsId && isResponse) {
            udpDone(true);
            return;
        }
    }
}

void ProxyCheckerDialog::udpDone(bool ok, const QString &detail)
{
    if (!m_udpPending)
        return;
    m_udpPending = false;
    if (ok)
        out(tr("UDP ASSOCIATE: %1 supported (a DNS reply came back through the relay)").arg(kOk));
    else
        out(tr("UDP ASSOCIATE: %1 not available%2")
                .arg(kFailed, detail.isEmpty() ? QString() : " - " + detail.toHtmlEscaped()));
    if (m_udp) {
        m_udp->abort();
        m_udp->deleteLater();
    }
    if (m_udpCtrl) {
        m_udpCtrl->disconnect(this);
        m_udpCtrl->abort();
        m_udpCtrl->deleteLater();
    }
    finish(true);
}

void ProxyCheckerDialog::finish(bool ok)
{
    if (!m_running)
        return;
    m_running = false;
    out("");
    out(ok ? tr("<b>Testing Finished.</b> Proxy server is working correctly!")
           : tr("<b>Testing Finished.</b> <span style='color:#c0201a'>The proxy server test failed.</span>"));
    m_start->setEnabled(true);
    if (m_socket)
        m_socket->abort();
}
