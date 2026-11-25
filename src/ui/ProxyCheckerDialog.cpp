#include "ProxyCheckerDialog.h"
#include "ui_ProxyCheckerDialog.h"

#include "core/DnsClient.h"
#include "core/ProxyConnector.h"
#include "core/Utils.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkProxy>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
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

// STUN servers for the UDP reachability and NAT test. The user ticks presets
// and/or types custom ones; at least two different operators are needed so their
// public-endpoint reports can be compared for NAT type. Google is offered but off
// by default, as it is unreachable from mainland China.
static const char *kStunEnabledKey = "proxyChecker/stunEnabled";
// Versioned: the preset list changed (dead servers replaced), so an old saved
// selection must not pin the stale set; a fresh key gives the new defaults.
static const char *kStunPresetsKey = "proxyChecker/stunPresets2";
static const char *kStunCustomKey = "proxyChecker/stunCustom";

struct StunPreset {
    const char *name;   // short label on the checkbox
    const char *addr;   // host:port, shown in the tooltip
    bool defOn;         // ticked by default
};
static const StunPreset kStunPresetList[] = {
    {"Xiaomi", "stun.miwifi.com:3478", true},
    {"Bilibili", "stun.chat.bilibili.com:3478", true},
    {"Cloudflare", "stun.cloudflare.com:3478", true},
    {"Google", "stun.l.google.com:19302", true},
    {"Blackberry", "stun.voip.blackberry.com:3478", true},
};

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
    m_stunEdit = ui->stunEdit;
    m_port = ui->portSpin;
    m_output = ui->outputEdit;
    m_start = ui->startButton;

    // STUN group: a master enable (the group's own checkbox), a row of preset
    // servers (name on the box, address in the tooltip), and a custom field.
    const QSettings s = util::appSettings();
    ui->stunGroup->setChecked(s.value(kStunEnabledKey, true).toBool());
    const bool havePresetPref = s.contains(kStunPresetsKey);
    const QStringList saved = s.value(kStunPresetsKey).toString().split(';', Qt::SkipEmptyParts);
    for (const StunPreset &p : kStunPresetList) {
        const QString addr = QString::fromUtf8(p.addr);
        auto *cb = new QCheckBox(QString::fromUtf8(p.name), this);
        cb->setToolTip(addr);
        cb->setChecked(havePresetPref ? saved.contains(addr) : p.defOn);
        ui->stunPresetRow->addWidget(cb);
        m_stunPresets.append(qMakePair(cb, addr));
    }
    ui->stunPresetRow->addStretch(1);
    m_stunEdit->setText(s.value(kStunCustomKey).toString());

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
    ++m_run;
    saveStunSettings();
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
    m_socket->setProxy(QNetworkProxy::NoProxy);
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
    m_socket->setProxy(QNetworkProxy::NoProxy);
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
        QTimer::singleShot(10000, this, [this, run = m_run, ssl = QPointer<QSslSocket>(ssl)] {
            if (run == m_run && m_running && ssl && !ssl->isEncrypted()) {
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
    // The request asks the server to close after replying, so the reply and
    // the close can be reported together, the close (or a "remote host
    // closed" error) even first. Drop the earlier handlers, the TLS one that
    // fails on any error included, and read what arrived before judging.
    m_socket->disconnect(this);
    const auto takeReply = [this]() -> bool {
        if (m_httpDone)
            return true;
        if (m_socket)
            m_httpBuf += m_socket->readAll();
        const qsizetype eol = m_httpBuf.indexOf("\r\n");
        if (eol < 0)
            return false;
        out(tr("Response: %1 %2").arg(QString::fromLatin1(m_httpBuf.left(eol)).toHtmlEscaped(), kOk));
        m_httpDone = true;
        if (m_socket)
            m_socket->disconnect(this);
        afterTcpTests();
        return true;
    };
    const auto closed = [this, takeReply](const QString &failure) {
        if (takeReply())
            return;
        // Give a reply still on its way (a pending readyRead) a moment.
        QTimer::singleShot(300, this, [this, run = m_run, takeReply, failure] {
            if (run == m_run && m_running && !takeReply()) {
                out(failure);
                finish(false);
            }
        });
    };
    connect(m_socket, &QTcpSocket::readyRead, this, [takeReply] { takeReply(); });
    connect(m_socket, &QTcpSocket::disconnected, this, [this, closed] {
        closed(tr("%1 : connection closed without a response").arg(kFailed));
    });
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this, closed](QAbstractSocket::SocketError e) {
        closed(e == QAbstractSocket::RemoteHostClosedError
                   ? tr("%1 : connection closed without a response").arg(kFailed)
                   : QString("%1 : %2").arg(kFailed, m_socket->errorString().toHtmlEscaped()));
    });
    m_socket->write(QString("GET / HTTP/1.1\r\nHost: %1\r\nUser-Agent: WinRoute ProxyChecker\r\nConnection: close\r\n\r\n")
                        .arg(host)
                        .toLatin1());
    QTimer::singleShot(10000, this, [this, run = m_run] {
        if (run == m_run && m_running && !m_httpDone) {
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
    m_udpPhase = 0;
    m_relayPort = 0;
    m_udpPending = true;
    m_udpCtrl = new QTcpSocket(this);
    m_udpCtrl->setProxy(QNetworkProxy::NoProxy);
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
    QTimer::singleShot(12000, this, [this, run = m_run] {
        // Only fires while we are still waiting for the ASSOCIATE reply; once
        // the relay is granted m_relayPort is set and the probes take over.
        if (run == m_run && m_relayPort == 0)
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
    m_udp->setProxy(QNetworkProxy::NoProxy);
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
    QTimer::singleShot(5000, this, [this, run = m_run] {
        if (run == m_run && m_udpPending && m_udpPhase == 0)
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
        if (d.size() <= off)
            continue;
        const QByteArray payload = d.mid(off);
        if (m_udpPhase == 0) {   // DNS reply to the associate probe
            if (payload.size() < 3)
                continue;
            const quint16 id = (quint8(payload[0]) << 8) | quint8(payload[1]);
            const bool isResponse = quint8(payload[2]) & 0x80;
            if (id == m_dnsId && isResponse) {
                udpDnsOk();
                return;
            }
        } else {                 // STUN reply
            handleStunDatagram(payload);
            if (!m_udpPending)   // stunFinish() ran
                return;
        }
    }
}

void ProxyCheckerDialog::cleanupUdp()
{
    if (m_udp) {
        m_udp->abort();
        m_udp->deleteLater();
    }
    if (m_udpCtrl) {
        m_udpCtrl->disconnect(this);
        m_udpCtrl->abort();
        m_udpCtrl->deleteLater();
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
    cleanupUdp();
    finish(true);
}

// The associate probe (DNS to 8.8.8.8:53) succeeded. Rather than finishing here,
// go on to the STUN probe, which tests what the DNS round-trip does not: reaching
// an arbitrary host on a non-53 port, and the exit's NAT mapping type. The relay
// (m_udpCtrl) and the local UDP socket (m_udp) stay open for it.
void ProxyCheckerDialog::saveStunSettings()
{
    QStringList checked;
    for (const auto &pr : m_stunPresets)
        if (pr.first->isChecked())
            checked << pr.second;
    QSettings s = util::appSettings();
    s.setValue(kStunEnabledKey, ui->stunGroup->isChecked());
    s.setValue(kStunPresetsKey, checked.join(';'));
    s.setValue(kStunCustomKey, m_stunEdit->text());
}

void ProxyCheckerDialog::udpDnsOk()
{
    out(tr("UDP ASSOCIATE: %1 supported (a DNS reply came back through the relay)").arg(kOk));
    if (!ui->stunGroup->isChecked()) {
        // The STUN test is switched off; the associate test above is enough.
        m_udpPending = false;
        cleanupUdp();
        finish(true);
        return;
    }
    stunTest();
}

void ProxyCheckerDialog::stunTest()
{
    out("");
    out(tr("<b>UDP Reachability and NAT Test (STUN)</b>"));
    m_udpPhase = 1;
    m_stun.clear();
    m_stunReplies = 0;

    // Servers = the ticked presets plus any custom entries (host:port, separated
    // by ; , or whitespace).
    QStringList addrs;
    for (const auto &pr : m_stunPresets)
        if (pr.first->isChecked())
            addrs << pr.second;
    addrs += m_stunEdit->text().trimmed().split(QRegularExpression("[;,\\s]+"), Qt::SkipEmptyParts);

    for (const QString &tok : addrs) {
        StunTarget t;
        const int colon = tok.lastIndexOf(':');
        if (colon > 0) {
            t.name = tok.left(colon).trimmed();
            bool okp = false;
            const uint pv = tok.mid(colon + 1).trimmed().toUInt(&okp);
            t.port = (okp && pv >= 1 && pv <= 65535) ? quint16(pv) : 3478;
        } else {
            t.name = tok;
            t.port = 3478;
        }
        if (!t.name.isEmpty())
            m_stun << t;
    }

    if (m_stun.isEmpty()) {
        out(tr("STUN test skipped: no STUN server selected."));
        stunFinish();
        return;
    }

    m_stunResolving = m_stun.size();
    for (int i = 0; i < m_stun.size(); ++i) {
        out(tr("Resolving %1 ...").arg(m_stun[i].name));
        m_dns->resolve(m_stun[i].name, [this, i, run = m_run](const QList<QHostAddress> &addrs, const QString &error) {
            if (run != m_run || i >= m_stun.size())
                return;
            for (const QHostAddress &h : addrs)
                if (h.protocol() == QAbstractSocket::IPv4Protocol) {
                    m_stun[i].ip = h;
                    break;
                }
            if (m_stun[i].ip.isNull())
                out(tr("Could not resolve %1%2")
                        .arg(m_stun[i].name, error.isEmpty() ? QString() : " - " + error.toHtmlEscaped()));
            if (--m_stunResolving == 0)
                startStunProbes();
        }, false);
    }
}

void ProxyCheckerDialog::startStunProbes()
{
    if (!m_udpPending)
        return;
    int sent = 0;
    for (int i = 0; i < m_stun.size(); ++i)
        if (!m_stun[i].ip.isNull()) {
            sendStun(i);
            ++sent;
        }
    if (sent == 0) {
        out(tr("STUN test skipped: could not resolve any STUN server."));
        stunFinish();
        return;
    }
    QTimer::singleShot(6000, this, [this, run = m_run] {
        if (run == m_run && m_udpPending && m_udpPhase == 1)
            stunFinish();
    });
}

void ProxyCheckerDialog::sendStun(int idx)
{
    StunTarget &t = m_stun[idx];
    t.txid.resize(12);
    for (int k = 0; k < 12; ++k)
        t.txid[k] = char(QRandomGenerator::global()->bounded(256));

    // STUN Binding Request: type 0x0001, length 0, magic cookie, transaction id.
    QByteArray msg;
    auto be16 = [&msg](quint16 v) { msg.append(char(v >> 8)); msg.append(char(v & 0xff)); };
    be16(0x0001);
    be16(0);
    be16(0x2112);
    be16(0xA442);
    msg.append(t.txid);

    // Wrap it in a SOCKS5 UDP header targeting the STUN server (ATYP=IPv4).
    QByteArray pkt("\x00\x00\x00\x01", 4);
    const quint32 dst = qToBigEndian<quint32>(t.ip.toIPv4Address());
    pkt.append(reinterpret_cast<const char *>(&dst), 4);
    const quint16 dport = qToBigEndian<quint16>(t.port);
    pkt.append(reinterpret_cast<const char *>(&dport), 2);
    pkt.append(msg);

    out(tr("Sending a STUN request to %1:%2 (%3) through the relay ...")
            .arg(t.name)
            .arg(t.port)
            .arg(t.ip.toString()));
    m_udp->writeDatagram(pkt, m_relayAddr, m_relayPort);
}

void ProxyCheckerDialog::handleStunDatagram(const QByteArray &p)
{
    if (p.size() < 20)
        return;
    if (((quint8(p[0]) << 8) | quint8(p[1])) != 0x0101)   // Binding success response
        return;
    if (quint8(p[4]) != 0x21 || quint8(p[5]) != 0x12 || quint8(p[6]) != 0xA4 || quint8(p[7]) != 0x42)
        return;
    const QByteArray txid = p.mid(8, 12);
    int idx = -1;
    for (int i = 0; i < m_stun.size(); ++i)
        if (m_stun[i].txid == txid) {
            idx = i;
            break;
        }
    if (idx < 0 || m_stun[idx].replied)
        return;

    // Walk the attributes for XOR-MAPPED-ADDRESS (0x0020), falling back to the
    // legacy MAPPED-ADDRESS (0x0001). Both carry the public endpoint the server
    // saw, i.e. the proxy exit's address as the outside world sees it.
    QHostAddress mapped;
    quint16 mport = 0;
    const int end = qMin(int(p.size()), 20 + ((quint8(p[2]) << 8) | quint8(p[3])));
    for (int o = 20; o + 4 <= end;) {
        const quint16 at = (quint8(p[o]) << 8) | quint8(p[o + 1]);
        const quint16 al = (quint8(p[o + 2]) << 8) | quint8(p[o + 3]);
        const int av = o + 4;
        if (av + al > p.size())
            break;
        if ((at == 0x0020 || at == 0x0001) && al >= 8 && quint8(p[av + 1]) == 0x01) {
            quint16 rport = (quint8(p[av + 2]) << 8) | quint8(p[av + 3]);
            quint32 raddr = (quint32(quint8(p[av + 4])) << 24) | (quint32(quint8(p[av + 5])) << 16)
                | (quint32(quint8(p[av + 6])) << 8) | quint32(quint8(p[av + 7]));
            if (at == 0x0020) {
                rport ^= 0x2112;
                raddr ^= 0x2112A442u;
            }
            mapped = QHostAddress(raddr);
            mport = rport;
            if (at == 0x0020)
                break;   // prefer XOR-MAPPED-ADDRESS
        }
        o = av + ((al + 3) & ~3);   // attributes are padded to 4 bytes
    }

    m_stun[idx].replied = true;
    m_stun[idx].mappedAddr = mapped;
    m_stun[idx].mappedPort = mport;
    ++m_stunReplies;
    out(tr("%1 replied %2 - the exit is seen as %3:%4")
            .arg(m_stun[idx].name, kOk)
            .arg(mapped.toString())
            .arg(mport));

    int resolved = 0;
    for (const StunTarget &s : m_stun)
        if (!s.ip.isNull())
            ++resolved;
    if (m_stunReplies >= resolved)
        stunFinish();
}

void ProxyCheckerDialog::stunFinish()
{
    if (!m_udpPending)
        return;
    m_udpPending = false;

    int resolved = 0;
    QList<int> ok;
    for (int i = 0; i < m_stun.size(); ++i)
        if (!m_stun[i].ip.isNull()) {
            ++resolved;
            if (m_stun[i].replied)
                ok << i;
        }

    // A few STUN servers being unreachable is common; list them but do not fail
    // on them. Reachability counts as proven if any server answered.
    for (int i = 0; i < m_stun.size(); ++i)
        if (!m_stun[i].ip.isNull() && !m_stun[i].replied)
            out(tr("%1 - no reply").arg(m_stun[i].name));

    if (resolved == 0) {
        // Resolution failures were already reported in stunTest().
    } else if (ok.isEmpty()) {
        out(tr("UDP to an arbitrary host: %1 (0/%2 replied) - datagrams relayed, no reply. DNS may "
               "pass, but P2P/relay traffic (e.g. Steam) will not work.")
                .arg(kFailed)
                .arg(resolved));
    } else {
        out(tr("UDP to an arbitrary host: %1 (%2/%3 replied).")
                .arg(kOk)
                .arg(ok.size())
                .arg(resolved));
        if (ok.size() >= 2) {
            const StunTarget &a = m_stun[ok[0]];
            const StunTarget &b = m_stun[ok[1]];
            if (a.mappedAddr == b.mappedAddr && a.mappedPort == b.mappedPort)
                out(tr("NAT mapping: %1 - endpoint-independent (full-cone), %2:%3. P2P-capable.")
                        .arg(kOk)
                        .arg(a.mappedAddr.toString())
                        .arg(a.mappedPort));
            else
                out(tr("NAT mapping: %1 - address/port-dependent (symmetric), %2:%3 vs %4:%5. P2P unsupported.")
                        .arg(kFailed)
                        .arg(a.mappedAddr.toString())
                        .arg(a.mappedPort)
                        .arg(b.mappedAddr.toString())
                        .arg(b.mappedPort));
        } else if (resolved >= 2) {
            out(tr("NAT mapping: single reply; type undetermined."));
        }
    }

    cleanupUdp();
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
