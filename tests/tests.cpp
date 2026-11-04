// Self-contained tests for the core library: profile IO, rule matching,
// DNS messages and the proxy protocol handshakes (against in-process proxies).

#include "core/Capture.h"
#include "core/CertAuthority.h"
#include "core/DnsMessage.h"
#include "core/FakeDns.h"
#include "core/FlowTable.h"
#include "core/HttpParse.h"
#include "core/Profile.h"
#include "core/ProxyConnector.h"
#include "core/Relay.h"
#include "core/RuleEngine.h"
#include "core/UdpRelaySession.h"
#include "core/Utils.h"
#include "core/WinDivertPacket.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUdpSocket>
#include <QtEndian>
#include <cstdio>
#include <cstring>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        ++g_checks;                                                                 \
        if (!(cond)) {                                                              \
            ++g_failures;                                                           \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                           \
    } while (0)

// ---- profile ---------------------------------------------------------------

static const char *kProxifierProfile = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ProxifierProfile version="102" platform="Windows" product_id="0" product_minver="400">
	<Options>
		<Resolve>
			<AutoModeDetection enabled="true" />
			<ViaProxy enabled="false" />
			<BlockNonATypes enabled="false" />
			<ExclusionList OnlyFromListMode="false">%ComputerName%; localhost; *.local</ExclusionList>
			<DnsUdpMode>0</DnsUdpMode>
		</Resolve>
		<Encryption mode="basic" />
		<ConnectionLoopDetection enabled="true" resolve="true" />
		<Udp mode="mode_bypass" />
		<LeakPreventionMode enabled="false" />
		<ProcessOtherUsers enabled="false" />
		<ProcessServices enabled="false" />
		<HandleDirectConnections enabled="false" />
		<HttpProxiesSupport enabled="false" />
	</Options>
	<ProxyList>
		<Proxy id="100" type="SOCKS5">
			<Options>48</Options>
			<Port>1551</Port>
			<Address>100.76.71.103</Address>
		</Proxy>
	</ProxyList>
	<ChainList />
	<RuleList>
		<Rule enabled="true">
			<Action type="Direct" />
			<Targets>localhost; 127.0.0.1; %ComputerName%</Targets>
			<Name>Localhost</Name>
		</Rule>
		<Rule enabled="true">
			<Action type="Proxy">100</Action>
			<Applications>parsecd.exe; pservice.exe</Applications>
			<Name>新建</Name>
		</Rule>
		<Rule enabled="true">
			<Action type="Direct" />
			<Name>Default</Name>
		</Rule>
	</RuleList>
</ProxifierProfile>
)";

static void testProfile()
{
    Profile p;
    QString err;
    CHECK(p.fromXml(QByteArray(kProxifierProfile), &err));
    CHECK(p.proxies.size() == 1);
    CHECK(p.proxies[0].id == 100);
    CHECK(p.proxies[0].type == ProxyType::Socks5);
    CHECK(p.proxies[0].address == "100.76.71.103");
    CHECK(p.proxies[0].port == 1551);
    CHECK(p.rules.size() == 3);
    CHECK(p.rules[1].name == QString::fromUtf8("新建"));
    CHECK(p.rules[1].action.type == ActionType::Proxy && p.rules[1].action.id == 100);
    CHECK(p.options.dnsExclusionList == "%ComputerName%; localhost; *.local");
    CHECK(p.options.encryption == PasswordEncryption::Basic);

    // add a chain and an authenticated proxy, then round-trip
    ProxyServer https;
    https.id = p.nextId();
    https.type = ProxyType::Https;
    https.address = "proxy.example.com";
    https.port = 3128;
    https.authEnabled = true;
    https.username = "user";
    https.password = "p@ss word";
    p.proxies << https;
    ProxyChain chain;
    chain.id = p.nextId();
    chain.name = "Office";
    chain.type = ChainType::Redundancy;
    chain.members << ChainMember{100, true} << ChainMember{https.id, false};
    p.chains << chain;
    p.rules[0].ports = "80; 8000-9000";
    p.rules[1].action = {ActionType::Chain, chain.id};

    for (PasswordEncryption mode : {PasswordEncryption::Disabled, PasswordEncryption::Basic, PasswordEncryption::User}) {
        p.options.encryption = mode;
        Profile q;
        CHECK(q.fromXml(p.toXml(), &err));
        CHECK(q.proxies.size() == 2);
        CHECK(q.proxies[1].password == "p@ss word");
        CHECK(q.proxies[1].username == "user");
        CHECK(q.proxies[1].type == ProxyType::Https);
        CHECK(q.chains.size() == 1);
        CHECK(q.chains[0].type == ChainType::Redundancy);
        CHECK(q.chains[0].members.size() == 2 && !q.chains[0].members[1].enabled);
        CHECK(q.rules[1].action.type == ActionType::Chain && q.rules[1].action.id == chain.id);
        CHECK(q.rules[0].ports == "80; 8000-9000");
        CHECK(q.rules[1].name == QString::fromUtf8("新建"));
    }

    // Application names with spaces must be quoted on export, or Proxifier
    // rejects the profile ("Unexpected blank space found"). The quotes must be
    // literal: Proxifier does not decode &quot;. They round-trip back unquoted,
    // and a plain name is left as-is.
    p.rules[1].applications = "chrome.exe; C:\\Program Files\\App\\a b.exe";
    const QByteArray xml = p.toXml();
    CHECK(xml.contains("\"C:\\Program Files\\App\\a b.exe\""));
    CHECK(!xml.contains("&quot;"));
    Profile q;
    CHECK(q.fromXml(xml, &err));
    CHECK(q.rules[1].applications == "chrome.exe; \"C:\\Program Files\\App\\a b.exe\"");
    CHECK(util::splitList(q.rules[1].applications)
          == (QStringList{"chrome.exe", "C:\\Program Files\\App\\a b.exe"}));
}

// ---- rules -----------------------------------------------------------------

static void testRules()
{
    CHECK(util::wildcardMatch(u"*.example.com", u"www.Example.com"));
    CHECK(!util::wildcardMatch(u"*.example.com", u"example.com"));
    CHECK(util::wildcardMatch(u"fire*.exe", u"firefox.exe"));
    CHECK(util::wildcardMatch(u"192.168.1.*", u"192.168.1.20"));
    CHECK(util::wildcardMatch(u"a?c", u"abc"));
    CHECK(!util::wildcardMatch(u"a?c", u"abbc"));

    CHECK((util::splitList("a.exe; \"C:\\some app.exe\" ;b") == QStringList{"a.exe", "C:\\some app.exe", "b"}));

    // joinAppList quotes names with spaces, leaves plain ones, and is idempotent.
    CHECK(util::joinAppList("chrome.exe\nC:\\some app.exe") == "chrome.exe; \"C:\\some app.exe\"");
    CHECK(util::joinAppList("a.exe; b.exe") == "a.exe; b.exe");
    CHECK(util::joinAppList("\"C:\\some app.exe\"") == "\"C:\\some app.exe\"");
    CHECK(util::joinAppList("") == "");

    CHECK(RuleEngine::matchPortList("80; 8000-9000", 8080));
    CHECK(RuleEngine::matchPortList("80; 8000-9000", 80));
    CHECK(!RuleEngine::matchPortList("80; 8000-9000", 443));

    const QStringList hosts = util::splitList("127.0.0.1; *.example.com; 10.1.0.0-10.5.255.255; 172.16.0.0/12; ::1; localhost");
    CHECK(RuleEngine::matchHostList(hosts, "", QHostAddress("10.3.4.5")));
    CHECK(!RuleEngine::matchHostList(hosts, "", QHostAddress("10.6.0.1")));
    CHECK(RuleEngine::matchHostList(hosts, "", QHostAddress("172.20.1.1")));
    CHECK(RuleEngine::matchHostList(hosts, "mail.example.com", QHostAddress("8.8.8.8")));
    CHECK(RuleEngine::matchHostList(hosts, "mail.example.com", QHostAddress()));
    CHECK(!RuleEngine::matchHostList(hosts, "example.org", QHostAddress("8.8.8.8")));
    CHECK(RuleEngine::matchHostList(hosts, "", QHostAddress("::ffff:127.0.0.1")));
    CHECK(RuleEngine::matchHostList(hosts, "localhost", QHostAddress()));

    const QStringList apps = util::splitList("chrome.exe; \"C:\\Program Files\\App\\a b.exe\"; fire*.exe");
    CHECK(RuleEngine::matchAppList(apps, "C:\\x\\Chrome.exe", "Chrome.exe"));
    CHECK(RuleEngine::matchAppList(apps, "C:\\Program Files\\App\\a b.exe", "a b.exe"));
    CHECK(!RuleEngine::matchAppList(apps, "D:\\Program Files\\App\\a b.exe", "a b.exe"));
    CHECK(RuleEngine::matchAppList(apps, "C:\\ff\\firefox.exe", "firefox.exe"));

    // A folder pattern (trailing backslash) matches every program under it,
    // including in subfolders, but not a sibling folder with the same prefix.
    const QStringList folder = util::splitList("\"C:\\Games\\\"");
    CHECK(RuleEngine::matchAppList(folder, "C:\\Games\\game.exe", "game.exe"));
    CHECK(RuleEngine::matchAppList(folder, "C:\\Games\\Steam\\app\\x.exe", "x.exe"));
    CHECK(RuleEngine::matchAppList(folder, "c:/games/sub/y.exe", "y.exe"));   // slashes, case
    CHECK(!RuleEngine::matchAppList(folder, "C:\\GamesOld\\z.exe", "z.exe"));
    CHECK(!RuleEngine::matchAppList(folder, "D:\\Games\\z.exe", "z.exe"));

    Profile p;
    p.fromXml(QByteArray(kProxifierProfile));
    RuleEngine engine(p);
    ConnectionRequest req;
    req.appName = "parsecd.exe";
    req.appPath = "C:\\Program Files\\Parsec\\parsecd.exe";
    req.ip = QHostAddress("1.2.3.4");
    req.port = 443;
    RuleMatch m = engine.match(req);
    CHECK(m.ruleIndex == 1 && m.action.type == ActionType::Proxy && m.action.id == 100);
    req.ip = QHostAddress("127.0.0.1");
    CHECK(engine.match(req).ruleIndex == 0);
    req.appName = "chrome.exe";
    req.ip = QHostAddress("1.2.3.4");
    m = engine.match(req);
    CHECK(m.ruleIndex == 2 && m.ruleName == "Default" && m.action.type == ActionType::Direct);

    // disabled rule is skipped
    p.rules[1].enabled = false;
    RuleEngine engine2(p);
    req.appName = "parsecd.exe";
    CHECK(engine2.match(req).ruleIndex == 2);
}

// ---- DNS -------------------------------------------------------------------

static void testDns()
{
    const QByteArray query = dns::buildQuery(0x1234, "www.example.com", dns::TypeA);
    dns::Question q;
    CHECK(dns::parseQuestion(query, &q));
    CHECK(q.id == 0x1234 && q.name == "www.example.com" && q.qtype == dns::TypeA);

    const QByteArray resp = dns::buildResponse(query, q, {QHostAddress("198.18.0.5"), QHostAddress("2001:db8::1")}, 30);
    dns::Question rq;
    QList<dns::Answer> answers;
    CHECK(dns::parseAnswers(resp, &rq, &answers));
    CHECK(rq.id == 0x1234 && (rq.flags & 0x8000));
    CHECK(answers.size() == 2);
    CHECK(answers[0].type == dns::TypeA && answers[0].address == QHostAddress("198.18.0.5") && answers[0].ttl == 30);
    CHECK(answers[1].type == dns::TypeAAAA && answers[1].address == QHostAddress("2001:db8::1"));

    FakeDns fake;
    const QHostAddress a = fake.addressFor("Example.COM");
    CHECK(FakeDns::isFake(a));
    CHECK(fake.addressFor("example.com") == a);
    CHECK(fake.hostnameFor(a) == "example.com");
    CHECK(fake.addressFor("other.com") != a);
    CHECK(!FakeDns::isFake(QHostAddress("8.8.8.8")));
    CHECK(FakeDns::isFake(QHostAddress("198.19.255.1")));
}

// ---- proxy protocols -------------------------------------------------------

// Minimal SOCKS4/4A/5 + HTTP CONNECT proxy that forwards to 127.0.0.1 targets.
class TestProxy : public QTcpServer {
public:
    enum Kind { Socks5, Socks4, Http };
    TestProxy(Kind kind, const QByteArray &user = {}, const QByteArray &pass = {})
        : m_kind(kind), m_user(user), m_pass(pass)
    {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *c = nextPendingConnection())
                handle(c);
        });
    }
    QStringList requests;

private:
    void handle(QTcpSocket *c)
    {
        auto *buf = new QByteArray;
        auto *stage = new int(0);
        connect(c, &QTcpSocket::readyRead, c, [=] {
            *buf += c->readAll();
            process(c, buf, stage);
        });
        connect(c, &QTcpSocket::disconnected, c, [=] {
            delete buf;
            delete stage;
            c->deleteLater();
        });
    }

    void tunnel(QTcpSocket *c, const QString &host, quint16 port, QByteArray *buf, const QByteArray &okReply)
    {
        requests << QString("%1:%2").arg(host).arg(port);
        auto *up = new QTcpSocket(c);
        const QHostAddress addr = (host == "localhost" || host == "echo.test") ? QHostAddress(QHostAddress::LocalHost) : QHostAddress(host);
        connect(up, &QTcpSocket::connected, c, [=] {
            c->write(okReply);
            if (!buf->isEmpty()) {
                up->write(*buf);
                buf->clear();
            }
            disconnect(c, &QTcpSocket::readyRead, nullptr, nullptr);
            connect(c, &QTcpSocket::readyRead, up, [=] { up->write(c->readAll()); });
            connect(up, &QTcpSocket::readyRead, c, [=] { c->write(up->readAll()); });
            connect(up, &QTcpSocket::disconnected, c, [=] { c->disconnectFromHost(); });
        });
        up->connectToHost(addr, port);
    }

    void process(QTcpSocket *c, QByteArray *buf, int *stage)
    {
        if (m_kind == Http) {
            const int end = buf->indexOf("\r\n\r\n");
            if (end < 0)
                return;
            const QByteArray head = buf->left(end);
            buf->remove(0, end + 4);
            if (!m_user.isEmpty() && !head.contains("Proxy-Authorization: Basic " + (m_user + ":" + m_pass).toBase64())) {
                c->write("HTTP/1.1 407 Proxy Authentication Required\r\n\r\n");
                c->disconnectFromHost();
                return;
            }
            const QByteArray authority = head.split(' ').value(1);
            const int colon = authority.lastIndexOf(':');
            tunnel(c, QString::fromLatin1(authority.left(colon)), quint16(authority.mid(colon + 1).toUInt()), buf,
                   "HTTP/1.1 200 Connection established\r\n\r\n");
            return;
        }
        if (m_kind == Socks4) {
            if (buf->size() < 9)
                return;
            const int nul = buf->indexOf('\0', 8);
            if (nul < 0)
                return;
            const quint16 port = qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(buf->constData() + 2));
            const quint32 ip = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(buf->constData() + 4));
            QString host;
            int consumed = nul + 1;
            if (ip > 0 && ip < 256) {
                const int nul2 = buf->indexOf('\0', nul + 1);
                if (nul2 < 0)
                    return;
                host = QString::fromLatin1(buf->mid(nul + 1, nul2 - nul - 1));
                consumed = nul2 + 1;
            } else {
                host = QHostAddress(ip).toString();
            }
            buf->remove(0, consumed);
            tunnel(c, host, port, buf, QByteArray("\x00\x5A\x00\x00\x00\x00\x00\x00", 8));
            return;
        }
        // SOCKS5
        if (*stage == 0) {
            if (buf->size() < 2 || buf->size() < 2 + quint8((*buf)[1]))
                return;
            const QByteArray methods = buf->mid(2, quint8((*buf)[1]));
            buf->remove(0, 2 + methods.size());
            if (!m_user.isEmpty()) {
                if (!methods.contains('\x02')) {
                    c->write("\x05\xFF");
                    return;
                }
                c->write("\x05\x02");
                *stage = 1;
            } else {
                c->write(QByteArray("\x05\x00", 2));
                *stage = 2;
            }
        }
        if (*stage == 1) {
            if (buf->size() < 2)
                return;
            const int ulen = quint8((*buf)[1]);
            if (buf->size() < 3 + ulen || buf->size() < 3 + ulen + quint8((*buf)[2 + ulen]))
                return;
            const QByteArray u = buf->mid(2, ulen);
            const int plen = quint8((*buf)[2 + ulen]);
            const QByteArray pw = buf->mid(3 + ulen, plen);
            buf->remove(0, 3 + ulen + plen);
            const bool ok = u == m_user && pw == m_pass;
            c->write(ok ? QByteArray("\x01\x00", 2) : QByteArray("\x01\x01", 2));
            if (!ok)
                return;
            *stage = 2;
        }
        if (*stage == 2) {
            if (buf->size() < 5)
                return;
            const quint8 atyp = quint8((*buf)[3]);
            QString host;
            int off;
            if (atyp == 1) {
                if (buf->size() < 10)
                    return;
                host = QHostAddress(qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(buf->constData() + 4))).toString();
                off = 8;
            } else if (atyp == 3) {
                const int len = quint8((*buf)[4]);
                if (buf->size() < 5 + len + 2)
                    return;
                host = QString::fromLatin1(buf->mid(5, len));
                off = 5 + len;
            } else {
                if (buf->size() < 22)
                    return;
                host = QHostAddress(reinterpret_cast<const quint8 *>(buf->constData() + 4)).toString();
                off = 20;
            }
            const quint16 port = qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(buf->constData() + off));
            buf->remove(0, off + 2);
            *stage = 3;
            tunnel(c, host, port, buf, QByteArray("\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00", 10));
        }
    }

    Kind m_kind;
    QByteArray m_user, m_pass;
};

class EchoServer : public QTcpServer {
public:
    EchoServer()
    {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *c = nextPendingConnection()) {
                connect(c, &QTcpSocket::readyRead, c, [c] { c->write(c->readAll()); });
                connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
            }
        });
    }
};

static ProxyServer proxyFor(const TestProxy &p, ProxyType type, const QString &user = {}, const QString &pass = {})
{
    ProxyServer s;
    s.type = type;
    s.address = "127.0.0.1";
    s.port = p.serverPort();
    s.authEnabled = !user.isEmpty();
    s.username = user;
    s.password = pass;
    return s;
}

// Runs a tunnel and checks that an echo round-trip works; returns the error, if any.
static QString runTunnel(const QList<ProxyServer> &hops, const Endpoint &target)
{
    QTcpSocket sock;
    ProxyConnector conn(&sock, hops, target, nullptr, 5000);
    QString error;
    bool done = false, ok = false;
    QObject::connect(&conn, &ProxyConnector::connected, [&](const QByteArray &) {
        sock.write("ping-through-tunnel");
        ok = true;
    });
    QObject::connect(&conn, &ProxyConnector::failed, [&](const QString &e) {
        error = e;
        done = true;
    });
    conn.start();
    QByteArray echoed;
    QElapsedTimer t;
    t.start();
    while (!done && t.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (ok)
            echoed += sock.readAll();
        if (echoed == "ping-through-tunnel")
            return {};
    }
    return error.isEmpty() ? QString("timeout") : error;
}

static void testProxies()
{
    EchoServer echo;
    Endpoint byIp{{}, QHostAddress(QHostAddress::LocalHost), echo.serverPort()};
    Endpoint byName{"echo.test", QHostAddress(QHostAddress::LocalHost), echo.serverPort()};
    Endpoint nameOnly{"echo.test", {}, echo.serverPort()};

    TestProxy socks5(TestProxy::Socks5);
    TestProxy socks5auth(TestProxy::Socks5, "bob", "secret");
    TestProxy socks4(TestProxy::Socks4);
    TestProxy http(TestProxy::Http);
    TestProxy httpAuth(TestProxy::Http, "alice", "pw");

    QString e;
    CHECK((e = runTunnel({}, byIp)).isEmpty());
    CHECK((e = runTunnel({proxyFor(socks5, ProxyType::Socks5)}, byIp)).isEmpty());
    CHECK((e = runTunnel({proxyFor(socks5, ProxyType::Socks5)}, nameOnly)).isEmpty());
    CHECK(socks5.requests.contains(QString("echo.test:%1").arg(echo.serverPort())));
    CHECK((e = runTunnel({proxyFor(socks5auth, ProxyType::Socks5, "bob", "secret")}, byIp)).isEmpty());
    e = runTunnel({proxyFor(socks5auth, ProxyType::Socks5, "bob", "wrong")}, byIp);
    CHECK(e.contains("authentication failed"));
    e = runTunnel({proxyFor(socks5auth, ProxyType::Socks5)}, byIp);
    CHECK(e.contains("requires authentication"));
    CHECK((e = runTunnel({proxyFor(socks4, ProxyType::Socks4)}, byIp)).isEmpty());
    CHECK((e = runTunnel({proxyFor(socks4, ProxyType::Socks4)}, byName)).isEmpty());   // SOCKS4A
    CHECK((e = runTunnel({proxyFor(http, ProxyType::Https)}, byName)).isEmpty());
    CHECK((e = runTunnel({proxyFor(httpAuth, ProxyType::Https, "alice", "pw")}, byIp)).isEmpty());
    e = runTunnel({proxyFor(httpAuth, ProxyType::Https)}, byIp);
    CHECK(e.contains("407"));
    // chain: SOCKS5 -> HTTPS -> SOCKS4 -> target
    CHECK((e = runTunnel({proxyFor(socks5, ProxyType::Socks5), proxyFor(http, ProxyType::Https),
                          proxyFor(socks4, ProxyType::Socks4)}, byName)).isEmpty());
    if (!e.isEmpty())
        std::fprintf(stderr, "  chain error: %s\n", qPrintable(e));
    // unreachable proxy
    ProxyServer dead;
    dead.address = "127.0.0.1";
    dead.port = 1;
    e = runTunnel({dead}, byIp);
    CHECK(e.startsWith("Could not connect to proxy 127.0.0.1:1"));
}

// ---- UDP over SOCKS5 -------------------------------------------------------

// A sockaddr_in as the driver passes it: family(LE) port(BE) addr(BE) + padding.
static QByteArray makeSockaddr4(quint32 ipv4, quint16 port)
{
    QByteArray sa(16, '\0');
    sa[0] = 2;   // AF_INET
    const quint16 bePort = qToBigEndian(port);
    memcpy(sa.data() + 2, &bePort, 2);
    const quint32 beIp = qToBigEndian(ipv4);
    memcpy(sa.data() + 4, &beIp, 4);
    return sa;
}

// A SOCKS5 proxy that supports UDP ASSOCIATE and echoes UDP datagrams back,
// keeping the SOCKS5 UDP header (so the reply appears to come from the target).
class TestUdpProxy : public QTcpServer {
public:
    TestUdpProxy()
    {
        m_udp = new QUdpSocket(this);
        m_udp->bind(QHostAddress::LocalHost, 0);
        m_relayPort = m_udp->localPort();
        connect(m_udp, &QUdpSocket::readyRead, this, [this] {
            while (m_udp->hasPendingDatagrams()) {
                QByteArray buf(int(m_udp->pendingDatagramSize()), '\0');
                QHostAddress from;
                quint16 fp = 0;
                m_udp->readDatagram(buf.data(), buf.size(), &from, &fp);
                if (buf.size() < 4)
                    continue;
                const quint8 atyp = quint8(buf[3]);
                int hdr;
                if (atyp == 1)
                    hdr = 10;
                else if (atyp == 4)
                    hdr = 22;
                else if (atyp == 3)
                    hdr = 5 + quint8(buf[4]) + 2;
                else
                    continue;
                if (buf.size() < hdr)
                    continue;
                lastAtyp = atyp;
                m_udp->writeDatagram(buf, from, fp);   // echo header + payload
            }
        });
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *c = nextPendingConnection())
                handle(c);
        });
    }
    quint16 relayPort() const { return m_relayPort; }
    int lastAtyp = 0;

private:
    void handle(QTcpSocket *c)
    {
        auto *buf = new QByteArray;
        auto *stage = new int(0);
        connect(c, &QTcpSocket::readyRead, c, [=] {
            *buf += c->readAll();
            process(c, buf, stage);
        });
        connect(c, &QTcpSocket::disconnected, c, [=] {
            delete buf;
            delete stage;
            c->deleteLater();
        });
    }
    void process(QTcpSocket *c, QByteArray *buf, int *stage)
    {
        if (*stage == 0) {
            if (buf->size() < 2 || buf->size() < 2 + quint8((*buf)[1]))
                return;
            buf->remove(0, 2 + quint8((*buf)[1]));
            c->write(QByteArray("\x05\x00", 2));   // no auth
            *stage = 2;
        }
        if (*stage == 2) {
            if (buf->size() < 5)
                return;
            const quint8 atyp = quint8((*buf)[3]);
            int off;
            if (atyp == 1) {
                if (buf->size() < 10)
                    return;
                off = 8;
            } else if (atyp == 3) {
                const int l = quint8((*buf)[4]);
                if (buf->size() < 5 + l + 2)
                    return;
                off = 5 + l;
            } else {
                if (buf->size() < 22)
                    return;
                off = 20;
            }
            buf->remove(0, off + 2);
            *stage = 3;
            QByteArray rep("\x05\x00\x00\x01", 4);   // success, BND = 127.0.0.1:relayPort
            const quint32 beIp = qToBigEndian<quint32>(0x7F000001);
            rep.append(reinterpret_cast<const char *>(&beIp), 4);
            const quint16 bePort = qToBigEndian(m_relayPort);
            rep.append(reinterpret_cast<const char *>(&bePort), 2);
            c->write(rep);
        }
    }
    QUdpSocket *m_udp;
    quint16 m_relayPort = 0;
};

// Drives one UDP datagram through a UdpRelaySession and returns the delivered
// payload (empty on timeout). gotRemote receives the source handed back.
static QByteArray runUdp(Relay *relay, const ProxyServer &proxy, const UdpDatagram &dg, QByteArray *gotRemote,
                         bool *bypassed = nullptr)
{
    QByteArray payload;
    auto *s = new UdpRelaySession(relay, dg, proxy);
    s->setSinks([&](const QByteArray &r, const QByteArray &pl) { if (gotRemote) *gotRemote = r; payload = pl; },
                [&](const UdpDatagram &) { if (bypassed) *bypassed = true; });
    QElapsedTimer t;
    t.start();
    while (payload.isEmpty() && (!bypassed || !*bypassed) && t.elapsed() < 5000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    s->close();
    QCoreApplication::processEvents();
    return payload;
}

static void testUdpProxy()
{
    FlowTable flows;
    Relay relay(&flows);
    TestUdpProxy proxy;
    Capture cap;
    cap.setEnabled(true);
    relay.setCapture(&cap);

    ProxyServer p;
    p.type = ProxyType::Socks5;
    p.address = "127.0.0.1";
    p.port = proxy.serverPort();

    // IPv4 target: wrapped with ATYP=1, echoed back as from the same address.
    UdpDatagram dg;
    dg.id = 1;
    dg.remote = makeSockaddr4(0x7F000001, 9999);
    dg.data = "ping-udp";
    QByteArray gotRemote;
    CHECK(runUdp(&relay, p, dg, &gotRemote) == "ping-udp");
    CHECK(gotRemote == dg.remote);
    CHECK(proxy.lastAtyp == 1);
    // The tunnelled datagram and its reply are one UDP entry in the capture.
    {
        const QList<CaptureSummary> rows = cap.summaries();
        CHECK(rows.size() == 1);
        if (!rows.isEmpty()) {
            const CaptureEntry e = cap.entry(rows.first().meta.id);
            CHECK(e.meta.udp && e.proto == "UDP");
            CHECK(e.meta.target == "127.0.0.1:9999");
            CHECK(e.segs.size() == 2);
            if (e.segs.size() == 2) {
                CHECK(e.segs[0].dir == CaptureDir::Up && e.segs[0].data == "ping-udp");
                CHECK(e.segs[1].dir == CaptureDir::Down && e.segs[1].data == "ping-udp");
            }
        }
    }

    // Fake-DNS target: sent by host name with ATYP=3, delivered as from the fake IP.
    UdpDatagram dgn;
    dgn.id = 2;
    dgn.remote = makeSockaddr4(0xC6120005, 5353);   // 198.18.0.5 (a fake DNS address)
    dgn.hostname = "echo.test";
    dgn.fakeIp = true;
    dgn.data = "by-name";
    QByteArray gotRemote2;
    CHECK(runUdp(&relay, p, dgn, &gotRemote2) == "by-name");
    CHECK(gotRemote2 == dgn.remote);
    CHECK(proxy.lastAtyp == 3);
    // A fake-DNS target is captured by host name, without the placeholder IP.
    {
        bool found = false;
        for (const CaptureSummary &r : cap.summaries())
            if (r.meta.hostname == "echo.test") {
                found = true;
                CHECK(r.meta.dstAddr.isNull());
                CHECK(r.meta.target == "echo.test:5353");
                CHECK(r.up == 7 && r.down == 7);
            }
        CHECK(found);
    }
    relay.setCapture(nullptr);

    // A proxy that is not reachable falls back to sending directly (bypass).
    ProxyServer dead;
    dead.type = ProxyType::Socks5;
    dead.address = "127.0.0.1";
    dead.port = 1;
    UdpDatagram dgd;
    dgd.id = 3;
    dgd.remote = makeSockaddr4(0x7F000001, 9999);
    dgd.data = "x";
    bool bypassed = false;
    QByteArray none;
    runUdp(&relay, dead, dgd, &none, &bypassed);
    CHECK(bypassed);
}

// ---- WinDivert packet rewriting -------------------------------------------

// Folds a 16-bit ones-complement sum over bytes read big-endian; a correct
// checksum makes the sum over the covered area (checksum field included) 0xFFFF.
static quint16 onesSum(const unsigned char *p, int n, quint32 acc = 0)
{
    for (; n > 1; n -= 2, p += 2)
        acc += (quint32(p[0]) << 8) | p[1];
    if (n)
        acc += quint32(p[0]) << 8;
    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);
    return quint16(acc);
}

static void testWinDivertPacket()
{
    // --- IPv4 TCP SYN -------------------------------------------------------
    unsigned char pkt[40] = {
        0x45, 0x00, 0x00, 0x28,            // ver/ihl, tos, total length 40
        0x12, 0x34, 0x00, 0x00,            // id, flags/frag
        0x40, 0x06, 0x00, 0x00,            // ttl, proto=TCP, checksum
        10, 0, 0, 5,                       // src 10.0.0.5
        93, 184, 216, 34,                  // dst 93.184.216.34
        0x30, 0x39, 0x01, 0xbb,            // sport 12345, dport 443
        0x00, 0x00, 0x00, 0x01,            // seq
        0x00, 0x00, 0x00, 0x00,            // ack
        0x50, 0x02, 0xfa, 0xf0,            // data offset 5, SYN, window
        0x00, 0x00, 0x00, 0x00,            // checksum, urg
    };
    CHECK(wdpkt::recalcTcpChecksums(pkt, sizeof(pkt)));
    CHECK(onesSum(pkt, 20) == 0xffff);              // IPv4 header checksum valid

    wdpkt::Endpoints ep;
    CHECK(wdpkt::readTcp(pkt, sizeof(pkt), &ep));
    CHECK(!ep.v6);
    CHECK(ep.srcPort == 12345 && ep.dstPort == 443);
    const unsigned char src[4] = {10, 0, 0, 5};
    const unsigned char dst[4] = {93, 184, 216, 34};
    CHECK(std::memcmp(ep.src, src, 4) == 0);
    CHECK(std::memcmp(ep.dst, dst, 4) == 0);

    // Redirect to the source IP:relayPort, preserving the source port.
    CHECK(wdpkt::setTcpDst(pkt, sizeof(pkt), src, 8888));
    CHECK(wdpkt::readTcp(pkt, sizeof(pkt), &ep));
    CHECK(std::memcmp(ep.dst, src, 4) == 0 && ep.dstPort == 8888 && ep.srcPort == 12345);
    CHECK(onesSum(pkt, 20) == 0xffff);
    {
        // The TCP checksum (pseudo-header + segment) must validate too.
        quint32 acc = 0;
        acc += onesSum(pkt + 12, 8);        // src + dst
        acc += 6 + 20;                      // protocol + TCP length
        CHECK(onesSum(pkt + 20, 20, acc) == 0xffff);
    }

    // Restore the original server as the source on the reply path.
    CHECK(wdpkt::setTcpSrc(pkt, sizeof(pkt), dst, 443));
    CHECK(wdpkt::readTcp(pkt, sizeof(pkt), &ep));
    CHECK(std::memcmp(ep.src, dst, 4) == 0 && ep.srcPort == 443);

    // --- IPv6 TCP -----------------------------------------------------------
    unsigned char p6[60] = {
        0x60, 0x00, 0x00, 0x00,            // version, traffic class, flow label
        0x00, 0x14, 0x06, 0x40,            // payload length 20, next header TCP, hop limit
        0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,   // src 2001:db8::1
        0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,   // dst 2001:db8::2
        0xd4, 0x31, 0x01, 0xbb,            // sport 54321, dport 443
        0, 0, 0, 1, 0, 0, 0, 0,
        0x50, 0x02, 0xfa, 0xf0,
        0x00, 0x00, 0x00, 0x00,
    };
    CHECK(wdpkt::recalcTcpChecksums(p6, sizeof(p6)));
    CHECK(wdpkt::readTcp(p6, sizeof(p6), &ep));
    CHECK(ep.v6 && ep.srcPort == 54321 && ep.dstPort == 443);
    {
        quint32 acc = 0;
        acc += onesSum(p6 + 8, 32);         // src + dst
        acc += 20 + 6;                      // TCP length + next header
        CHECK(onesSum(p6 + 40, 20, acc) == 0xffff);
    }

    // Non-TCP / too-short buffers are rejected.
    unsigned char bad[8] = {0x45, 0, 0, 8, 0, 0, 0, 0};
    CHECK(!wdpkt::readTcp(bad, sizeof(bad), &ep));

    // --- UDP build + parse round-trip (IPv4) ----------------------------------
    const unsigned char us[4] = {192, 168, 1, 10};
    const unsigned char ud[4] = {8, 8, 8, 8};
    const unsigned char msg[9] = {'h', 'e', 'l', 'l', 'o', '-', 'u', 'd', 'p'};
    unsigned char ub[64];
    const unsigned un = wdpkt::buildUdp(false, us, 5555, ud, 53, msg, sizeof(msg), ub, sizeof(ub));
    CHECK(un == 20 + 8 + sizeof(msg));
    CHECK(onesSum(ub, 20) == 0xffff);                 // IPv4 header checksum valid
    {
        // UDP checksum (pseudo-header + segment) valid.
        quint32 pseudo = (quint32(ub[12]) << 8 | ub[13]) + (quint32(ub[14]) << 8 | ub[15])
                         + (quint32(ub[16]) << 8 | ub[17]) + (quint32(ub[18]) << 8 | ub[19])
                         + 17u /*proto*/ + (8u + sizeof(msg)) /*udp length*/;
        CHECK(onesSum(ub + 20, int(8 + sizeof(msg)), pseudo) == 0xffff);
    }
    wdpkt::Endpoints uep;
    const unsigned char *up = nullptr;
    unsigned upl = 0;
    CHECK(wdpkt::readUdp(ub, un, &uep, &up, &upl));
    CHECK(!uep.v6 && uep.srcPort == 5555 && uep.dstPort == 53 && upl == sizeof(msg));
    CHECK(std::memcmp(up, msg, sizeof(msg)) == 0);
    CHECK(std::memcmp(uep.src, us, 4) == 0 && std::memcmp(uep.dst, ud, 4) == 0);

    // --- UDP build + parse round-trip (IPv6) ----------------------------------
    unsigned char us6[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0a};
    unsigned char ud6[16] = {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0, 0, 0, 0, 0, 0, 0, 0, 0x88, 0x88};
    unsigned char ub6[80];
    const unsigned un6 = wdpkt::buildUdp(true, us6, 6000, ud6, 53, msg, sizeof(msg), ub6, sizeof(ub6));
    CHECK(un6 == 40 + 8 + sizeof(msg));
    CHECK(wdpkt::readUdp(ub6, un6, &uep, &up, &upl));
    CHECK(uep.v6 && uep.srcPort == 6000 && uep.dstPort == 53 && upl == sizeof(msg));
    CHECK(std::memcmp(uep.src, us6, 16) == 0 && std::memcmp(uep.dst, ud6, 16) == 0);
}

// ---- capture, HTTP parsing and exports -------------------------------------

static CaptureSegment seg(CaptureDir dir, const char *data, qint64 ts = 0)
{
    return CaptureSegment{dir, ts, QByteArray(data)};
}

static void testCapture()
{
    // HTTP with Content-Length.
    {
        CaptureEntry e;
        e.proto = "HTTP";
        e.meta.srcAddr = QHostAddress("192.168.1.5");
        e.meta.srcPort = 50000;
        e.meta.dstAddr = QHostAddress("93.184.216.34");
        e.meta.dstPort = 80;
        e.meta.hostname = "example.com";
        e.meta.startMs = 1700000000000LL;
        e.segs << seg(CaptureDir::Up, "GET /x HTTP/1.1\r\nHost: example.com\r\n\r\n", 10)
               << seg(CaptureDir::Down, "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\n\r\nhello", 20);

        const QList<HttpExchange> xs = parseHttp(e);
        CHECK(xs.size() == 1);
        CHECK(xs[0].request.method == "GET");
        CHECK(xs[0].request.uri == "/x");
        CHECK(xs[0].response.statusCode == 200);
        CHECK(xs[0].response.body == "hello");
        CHECK(xs[0].response.complete);

        // HAR round-trips through JSON with the request/response preserved.
        const QByteArray har = Capture::toHar({e});
        const QJsonObject log = QJsonDocument::fromJson(har).object()["log"].toObject();
        const QJsonArray entries = log["entries"].toArray();
        CHECK(entries.size() == 1);
        const QJsonObject en = entries[0].toObject();
        CHECK(en["request"].toObject()["method"].toString() == "GET");
        CHECK(en["request"].toObject()["url"].toString() == "http://example.com/x");
        CHECK(en["response"].toObject()["status"].toInt() == 200);
    }

    // Chunked response body.
    {
        CaptureEntry e;
        e.segs << seg(CaptureDir::Up, "GET / HTTP/1.1\r\nHost: h\r\n\r\n")
               << seg(CaptureDir::Down, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n");
        const QList<HttpExchange> xs = parseHttp(e);
        CHECK(xs.size() == 1);
        CHECK(xs[0].response.body == "hello world");
        CHECK(xs[0].response.complete);
    }

    // A non-HTTP stream yields no exchanges.
    {
        CaptureEntry e;
        e.segs << seg(CaptureDir::Up, "\x16\x03\x01\x00\x05hello");
        CHECK(parseHttp(e).isEmpty());
    }

    // PCAPNG export: Section Header Block magic and a plausible size.
    {
        CaptureEntry e;
        e.proto = "HTTP";
        e.meta.srcAddr = QHostAddress("192.168.1.5");
        e.meta.srcPort = 50000;
        e.meta.dstAddr = QHostAddress("93.184.216.34");
        e.meta.dstPort = 80;
        e.meta.startMs = 1700000000000LL;
        e.segs << seg(CaptureDir::Up, "GET / HTTP/1.1\r\n\r\n", 10)
               << seg(CaptureDir::Down, "HTTP/1.1 200 OK\r\n\r\n", 20);
        const QByteArray pcap = Capture::toPcapng({e});
        CHECK(pcap.size() > 100);
        // SHB block type 0x0A0D0D0A, written little-endian.
        CHECK(quint8(pcap[0]) == 0x0A && quint8(pcap[1]) == 0x0D
              && quint8(pcap[2]) == 0x0D && quint8(pcap[3]) == 0x0A);
        // Byte-order magic 0x1A2B3C4D at offset 8 (little-endian).
        CHECK(quint8(pcap[8]) == 0x4D && quint8(pcap[9]) == 0x3C
              && quint8(pcap[10]) == 0x2B && quint8(pcap[11]) == 0x1A);
    }

    // PCAPNG with unknown endpoints (src 0.0.0.0 at connect time, fake-DNS dst):
    // the placeholders must not leak through as 0.0.0.0 / a shared fallback, and
    // a Name Resolution Block must be present.
    {
        CaptureEntry e;
        e.proto = "HTTP";
        e.meta.appName = "curl.exe";
        e.meta.srcAddr = QHostAddress(QHostAddress::AnyIPv4);   // 0.0.0.0
        e.meta.srcPort = 37205;
        e.meta.dstAddr = QHostAddress();                        // unknown (fake DNS)
        e.meta.dstPort = 80;
        e.meta.hostname = "example.com";
        e.meta.startMs = 1700000000000LL;
        e.segs << seg(CaptureDir::Up, "GET / HTTP/1.1\r\n\r\n", 10);
        const QByteArray pcap = Capture::toPcapng({e});

        // Walk the blocks to the first Enhanced Packet Block, and note the NRB.
        bool sawNrb = false;
        int off = 0, firstEpb = -1;
        while (off + 8 <= pcap.size()) {
            const quint32 type = quint8(pcap[off]) | (quint8(pcap[off + 1]) << 8)
                                 | (quint8(pcap[off + 2]) << 16) | (quint32(quint8(pcap[off + 3])) << 24);
            const quint32 len = quint8(pcap[off + 4]) | (quint8(pcap[off + 5]) << 8)
                                | (quint8(pcap[off + 6]) << 16) | (quint32(quint8(pcap[off + 7])) << 24);
            if (type == 0x00000004u)
                sawNrb = true;
            if (type == 0x00000006u) { firstEpb = off; break; }
            if (len < 12)
                break;
            off += len;
        }
        CHECK(sawNrb);
        CHECK(firstEpb >= 0);
        if (firstEpb >= 0) {
            const int ip = firstEpb + 28 + 14;   // EPB header + Ethernet
            CHECK(quint8(pcap[ip + 12]) == 10);   // client placeholder 10.x.x.x
            CHECK(quint8(pcap[ip + 16]) == 198);  // server placeholder 198.18.x.x
        }
    }

    // A decrypted HTTPS connection must not be exported on port 443: the payload
    // is plaintext, and Wireshark would otherwise dissect it as TLS and show a
    // garbled, incomplete stream. HTTP moves to port 80; a raw TLS capture keeps
    // 443 (it really is TLS there).
    {
        // Reads the first EPB's TCP destination port (the synthesized SYN goes
        // client -> server, so its destination port is the server port).
        auto firstTcpDstPort = [](const QByteArray &pcap) -> int {
            int off = 0;
            while (off + 8 <= pcap.size()) {
                const quint32 type = quint8(pcap[off]) | (quint8(pcap[off + 1]) << 8)
                                     | (quint8(pcap[off + 2]) << 16) | (quint32(quint8(pcap[off + 3])) << 24);
                const quint32 len = quint8(pcap[off + 4]) | (quint8(pcap[off + 5]) << 8)
                                    | (quint8(pcap[off + 6]) << 16) | (quint32(quint8(pcap[off + 7])) << 24);
                if (type == 0x00000006u) {
                    const int tcp = off + 28 + 14 + 20;   // EPB hdr + Ethernet + IPv4
                    return (quint8(pcap[tcp + 2]) << 8) | quint8(pcap[tcp + 3]);
                }
                if (len < 12)
                    break;
                off += len;
            }
            return -1;
        };

        CaptureEntry dec;
        dec.proto = "HTTP";
        dec.decrypted = true;
        dec.meta.srcAddr = QHostAddress("192.168.1.5");
        dec.meta.srcPort = 50000;
        dec.meta.dstAddr = QHostAddress("93.184.216.34");
        dec.meta.dstPort = 443;
        dec.segs << seg(CaptureDir::Up, "GET / HTTP/1.1\r\nHost: x\r\n\r\n", 10)
                 << seg(CaptureDir::Down, "HTTP/1.1 200 OK\r\n\r\n", 20);
        CHECK(firstTcpDstPort(Capture::toPcapng({dec})) == 80);
        // Keeping real ports leaves the decrypted stream on its true port.
        CHECK(firstTcpDstPort(Capture::toPcapng({dec}, true)) == 443);

        CaptureEntry raw = dec;
        raw.proto = "TLS";
        raw.decrypted = false;
        CHECK(firstTcpDstPort(Capture::toPcapng({raw})) == 443);
    }

    // Exported PDU: no block may exceed Wireshark's 262144-byte packet limit, so
    // a large run is split across several PDUs.
    {
        CaptureEntry big;
        big.proto = "HTTP";
        big.decrypted = true;
        big.meta.srcAddr = QHostAddress("10.0.0.2");
        big.meta.srcPort = 50000;
        big.meta.dstAddr = QHostAddress("93.184.216.34");
        big.meta.dstPort = 443;
        big.segs << seg(CaptureDir::Up, "GET / HTTP/1.1\r\n\r\n", 10)
                 << seg(CaptureDir::Down, QByteArray(1024 * 1024, 'x'), 20);   // 1 MB body
        const QByteArray pdu = Capture::toExportedPdu({big});
        int off = 0, maxCap = 0, epbs = 0;
        while (off + 8 <= pdu.size()) {
            const quint32 type = quint8(pdu[off]) | (quint8(pdu[off + 1]) << 8)
                                 | (quint8(pdu[off + 2]) << 16) | (quint32(quint8(pdu[off + 3])) << 24);
            const quint32 len = quint8(pdu[off + 4]) | (quint8(pdu[off + 5]) << 8)
                                | (quint8(pdu[off + 6]) << 16) | (quint32(quint8(pdu[off + 7])) << 24);
            if (type == 0x00000006u) {
                ++epbs;
                const int cap = quint8(pdu[off + 20]) | (quint8(pdu[off + 21]) << 8)
                                | (quint8(pdu[off + 22]) << 16) | (int(quint8(pdu[off + 23])) << 24);
                maxCap = qMax(maxCap, cap);
            }
            if (len < 12)
                break;
            off += len;
        }
        CHECK(epbs >= 6);               // 1 MB split into several PDUs
        CHECK(maxCap <= 262144);        // every one within Wireshark's limit
    }

    // Per-connection cap truncates and flags the entry.
    {
        Capture cap;
        cap.setEnabled(true);
        cap.setLimits(10, 0);
        CaptureMeta m;
        m.id = 1;
        cap.note(m);
        cap.append(1, CaptureDir::Up, QByteArray(100, 'a'), 0);
        const CaptureEntry e = cap.entry(1);
        CHECK(e.up == 10);
        CHECK(e.truncated);
    }

    // Capture filter: a list of rules applied as global / whitelist / blacklist.
    {
        Capture cap;
        const QHostAddress ip("93.184.216.34");

        CaptureFilter all;   // All: everything, rules ignored
        cap.setFilter(all);
        CHECK(cap.shouldCapture("chrome.exe", "", "example.com", ip, 443));

        CaptureFilter white;
        white.mode = CaptureFilter::Only;
        white.rules.append(CaptureRule{true, "chrome.exe", "", ""});
        cap.setFilter(white);
        CHECK(cap.shouldCapture("chrome.exe", "", "example.com", ip, 443));    // matches -> captured
        CHECK(!cap.shouldCapture("firefox.exe", "", "example.com", ip, 443));  // no match -> skipped

        CaptureFilter black;
        black.mode = CaptureFilter::Except;
        black.rules.append(CaptureRule{true, "", "", "443"});
        cap.setFilter(black);
        CHECK(!cap.shouldCapture("chrome.exe", "", "example.com", ip, 443));   // matches -> excluded
        CHECK(cap.shouldCapture("chrome.exe", "", "example.com", ip, 80));     // no match -> captured

        // A rule's fields are ANDed: target + port must both match.
        CaptureFilter hostPort;
        hostPort.mode = CaptureFilter::Only;
        hostPort.rules.append(CaptureRule{true, "", "*.example.com", "443"});
        cap.setFilter(hostPort);
        CHECK(cap.shouldCapture("x", "", "www.example.com", QHostAddress(), 443));
        CHECK(!cap.shouldCapture("x", "", "www.example.com", QHostAddress(), 80));   // port differs
        CHECK(!cap.shouldCapture("x", "", "other.org", QHostAddress(), 443));        // host differs

        // Multiple rules are ORed: a connection matching any rule is captured.
        CaptureFilter two;
        two.mode = CaptureFilter::Only;
        two.rules.append(CaptureRule{true, "chrome.exe", "", ""});
        two.rules.append(CaptureRule{true, "", "", "22"});
        cap.setFilter(two);
        CHECK(cap.shouldCapture("chrome.exe", "", "a.com", ip, 443));   // rule 1
        CHECK(cap.shouldCapture("ssh.exe", "", "a.com", ip, 22));       // rule 2
        CHECK(!cap.shouldCapture("ssh.exe", "", "a.com", ip, 443));     // neither

        // A disabled rule is ignored.
        CaptureFilter disabled;
        disabled.mode = CaptureFilter::Only;
        disabled.rules.append(CaptureRule{false, "chrome.exe", "", ""});
        cap.setFilter(disabled);
        CHECK(!cap.shouldCapture("chrome.exe", "", "a.com", ip, 443));

        // Empty whitelist captures nothing; empty blacklist captures everything.
        CaptureFilter emptyWhite;
        emptyWhite.mode = CaptureFilter::Only;
        cap.setFilter(emptyWhite);
        CHECK(!cap.shouldCapture("chrome.exe", "", "example.com", ip, 443));
        CaptureFilter emptyBlack;
        emptyBlack.mode = CaptureFilter::Except;
        cap.setFilter(emptyBlack);
        CHECK(cap.shouldCapture("chrome.exe", "", "example.com", ip, 443));
    }
}

// Certificates are inspected with OpenSSL directly (the same library the CA
// signs them with) rather than QSslCertificate, so the test verifies the real
// security properties: the leaf is signed by the root, carries the right SAN,
// and is not itself a CA.
namespace certtest {

X509 *parse(const QByteArray &pem)
{
    BIO *bio = BIO_new_mem_buf(pem.constData(), int(pem.size()));
    if (!bio)
        return nullptr;
    X509 *x = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return x;
}

QString commonName(X509_NAME *name)
{
    char buf[256] = {0};
    const int n = X509_NAME_get_text_by_NID(name, NID_commonName, buf, sizeof(buf));
    return n > 0 ? QString::fromUtf8(buf, n) : QString();
}

QStringList sans(X509 *x)
{
    QStringList out;
    auto *names = static_cast<GENERAL_NAMES *>(X509_get_ext_d2i(x, NID_subject_alt_name, nullptr, nullptr));
    if (!names)
        return out;
    for (int i = 0; i < sk_GENERAL_NAME_num(names); ++i) {
        const GENERAL_NAME *gn = sk_GENERAL_NAME_value(names, i);
        if (gn->type == GEN_DNS) {
            const auto *s = gn->d.dNSName;
            out << "DNS:" + QString::fromUtf8(reinterpret_cast<const char *>(ASN1_STRING_get0_data(s)),
                                              ASN1_STRING_length(s));
        } else if (gn->type == GEN_IPADD) {
            const auto *s = gn->d.iPAddress;
            if (ASN1_STRING_length(s) == 4) {
                const unsigned char *d = ASN1_STRING_get0_data(s);
                out << QString("IP:%1.%2.%3.%4").arg(d[0]).arg(d[1]).arg(d[2]).arg(d[3]);
            }
        }
    }
    GENERAL_NAMES_free(names);
    return out;
}

} // namespace certtest

// Reads the first Enhanced Packet Block's frame (Ethernet onwards), or empty.
static QByteArray firstEpbFrame(const QByteArray &pcap)
{
    int off = 0;
    while (off + 8 <= pcap.size()) {
        const quint32 type = quint8(pcap[off]) | (quint8(pcap[off + 1]) << 8)
                             | (quint8(pcap[off + 2]) << 16) | (quint32(quint8(pcap[off + 3])) << 24);
        const quint32 len = quint8(pcap[off + 4]) | (quint8(pcap[off + 5]) << 8)
                            | (quint8(pcap[off + 6]) << 16) | (quint32(quint8(pcap[off + 7])) << 24);
        if (type == 0x00000006u) {
            const int capLen = quint8(pcap[off + 20]) | (quint8(pcap[off + 21]) << 8)
                               | (quint8(pcap[off + 22]) << 16) | (quint8(pcap[off + 23]) << 24);
            return pcap.mid(off + 28, capLen);
        }
        if (len < 12)
            break;
        off += len;
    }
    return {};
}

static void testCaptureDatagrams()
{
    CaptureMeta base;
    base.pid = 1234;
    base.appName = "svchost.exe";
    base.srcAddr = QHostAddress("192.168.1.5");
    base.srcPort = 53001;
    base.dstAddr = QHostAddress("8.8.8.8");
    base.dstPort = 53;

    dns::Question q;
    q.name = "www.example.com";
    q.qtype = dns::TypeA;
    const QByteArray query = dns::buildQuery(0x1234, q.name, dns::TypeA);
    CHECK(dns::parseQuestion(query, &q));
    const QByteArray answer = dns::buildResponse(query, q, {QHostAddress("93.184.216.34")}, 300);

    // Off: nothing is recorded.
    {
        Capture cap;
        cap.dnsMessage(base, CaptureDir::Up, query, 1000);
        CHECK(cap.count() == 0);
    }

    // A query and its answer on one socket form one DNS entry.
    Capture cap;
    cap.setEnabled(true);
    cap.dnsMessage(base, CaptureDir::Up, query, 1000);
    cap.dnsMessage(base, CaptureDir::Down, answer, 1010);
    CHECK(cap.count() == 1);
    const QList<CaptureSummary> rows = cap.summaries();
    if (!rows.isEmpty()) {
        const CaptureSummary &r = rows.first();
        CHECK(r.proto == "DNS");
        CHECK(r.meta.udp);
        CHECK(!r.open);
        CHECK(!r.synthesized);
        CHECK(r.meta.target == "www.example.com @ 8.8.8.8:53");
        CHECK(r.meta.id & (quint64(1) << 63));   // apart from the relay's connection ids
        CHECK(r.up == query.size() && r.down == answer.size());
        const CaptureEntry e = cap.entry(r.meta.id);
        CHECK(e.segs.size() == 2);
    }

    // Another socket (source port) is another entry; a made-up answer is flagged.
    CaptureMeta other = base;
    other.srcPort = 53002;
    cap.dnsMessage(other, CaptureDir::Up, query, 2000);
    cap.dnsMessage(other, CaptureDir::Down, answer, 2001, true);
    CHECK(cap.count() == 2);
    bool sawSynth = false;
    for (const CaptureSummary &r : cap.summaries())
        sawSynth = sawSynth || (r.meta.srcPort == 53002 && r.synthesized);
    CHECK(sawSynth);

    // Clearing forgets the flows too: the same socket starts a fresh entry.
    cap.clear();
    CHECK(cap.count() == 0);
    cap.dnsMessage(base, CaptureDir::Up, query, 3000);
    CHECK(cap.count() == 1);

    // The capture filter matches DNS by the name asked for.
    {
        Capture f;
        f.setEnabled(true);
        CaptureFilter flt;
        flt.mode = CaptureFilter::Only;
        CaptureRule rule;
        rule.targets = "*.example.com";
        flt.rules << rule;
        f.setFilter(flt);
        f.dnsMessage(base, CaptureDir::Up, query, 1);
        CaptureMeta m2 = base;
        m2.srcPort = 53010;
        f.dnsMessage(m2, CaptureDir::Up, dns::buildQuery(7, "other.org", dns::TypeA), 2);
        CHECK(f.count() == 1);
    }

    // Exports: real UDP packets on port 53 in PCAPNG, "dns" PDUs, nothing in HAR.
    {
        Capture c;
        c.setEnabled(true);
        c.dnsMessage(base, CaptureDir::Up, query, 1000);
        c.dnsMessage(base, CaptureDir::Down, answer, 1010);
        const CaptureEntry e = c.entry(c.summaries().first().meta.id);

        const QByteArray frame = firstEpbFrame(Capture::toPcapng({e}));
        CHECK(frame.size() == 14 + 20 + 8 + query.size());   // Ethernet + IPv4 + UDP + query, no handshake
        if (frame.size() >= 14 + 20 + 8) {
            CHECK(quint8(frame[14 + 9]) == 17);                                       // IP protocol: UDP
            CHECK(((quint8(frame[34 + 2]) << 8) | quint8(frame[34 + 3])) == 53);     // destination port
            CHECK(((quint8(frame[34]) << 8) | quint8(frame[35])) == 53001);         // source port
            CHECK(frame.mid(42) == query);
        }

        const QByteArray pdu = Capture::toExportedPdu({e});
        CHECK(pdu.contains(QByteArray("dns")));
        CHECK(pdu.contains(query) && pdu.contains(answer));

        const QJsonObject har = QJsonDocument::fromJson(Capture::toHar({e})).object();
        CHECK(har["log"].toObject()["entries"].toArray().isEmpty());
    }
}

static void testCertAuthority()
{
    using namespace certtest;
    // Keep the generated CA out of the real %APPDATA% by pointing it at a temp
    // directory for the duration of the test.
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    const QByteArray savedAppData = qgetenv("APPDATA");
    qputenv("APPDATA", QDir::toNativeSeparators(tmp.path()).toUtf8());

    QByteArray caPem;
    {
        CertAuthority ca;
        CHECK(!ca.exists());
        QString err;
        CHECK(ca.ensureCa(&err));
        CHECK(ca.exists());

        caPem = ca.caCertPem();
        X509 *caCert = parse(caPem);
        CHECK(caCert != nullptr);
        if (caCert) {
            CHECK(commonName(X509_get_subject_name(caCert)) == QStringLiteral("WinRoute Root CA"));
            // Self-signed root: issuer == subject, and it is a CA.
            CHECK(commonName(X509_get_issuer_name(caCert)) == commonName(X509_get_subject_name(caCert)));
            CHECK(X509_check_ca(caCert) >= 1);
        }

        // A leaf for a host name: CN + DNS SAN, issued by and signed by the root,
        // and not itself a CA.
        const CertAuthority::Leaf leaf = ca.leafFor(QStringLiteral("www.example.com"));
        CHECK(!leaf.certPem.isEmpty());
        CHECK(leaf.keyPem.contains("PRIVATE KEY"));
        X509 *leafCert = parse(leaf.certPem);
        CHECK(leafCert != nullptr);
        if (leafCert && caCert) {
            CHECK(commonName(X509_get_subject_name(leafCert)) == QStringLiteral("www.example.com"));
            CHECK(commonName(X509_get_issuer_name(leafCert)) == QStringLiteral("WinRoute Root CA"));
            CHECK(sans(leafCert).contains(QStringLiteral("DNS:www.example.com")));
            CHECK(X509_check_ca(leafCert) == 0);
            // The signature verifies against the root's public key.
            EVP_PKEY *caPub = X509_get_pubkey(caCert);
            CHECK(caPub && X509_verify(leafCert, caPub) == 1);
            if (caPub)
                EVP_PKEY_free(caPub);
        }

        // A leaf for an IP literal gets an IP SAN instead.
        const CertAuthority::Leaf ipLeaf = ca.leafFor(QStringLiteral("10.1.2.3"));
        X509 *ipCert = parse(ipLeaf.certPem);
        CHECK(ipCert != nullptr);
        if (ipCert) {
            CHECK(sans(ipCert).contains(QStringLiteral("IP:10.1.2.3")));
            X509_free(ipCert);
        }

        // A repeat lookup is cached (same PEM).
        CHECK(ca.leafFor(QStringLiteral("www.example.com")).certPem == leaf.certPem);

        if (leafCert)
            X509_free(leafCert);
        if (caCert)
            X509_free(caCert);
    }
    {
        // A fresh instance loads the CA persisted above rather than making a new
        // one.
        CertAuthority reloaded;
        CHECK(reloaded.exists());
        CHECK(reloaded.caCertPem() == caPem);
    }
    {
        // createCa replaces the CA with a custom subject.
        CertAuthority ca;
        CertAuthority::CaInfo cfg;
        cfg.commonName = QStringLiteral("My Custom CA");
        cfg.organization = QStringLiteral("Acme Ltd");
        cfg.validityDays = 365;
        CHECK(ca.createCa(cfg, nullptr));
        CHECK(ca.caCertPem() != caPem);   // a new certificate

        const CertAuthority::CertDetails d = ca.caDetails();
        CHECK(d.valid);
        CHECK(d.commonName == QStringLiteral("My Custom CA"));
        CHECK(d.organization == QStringLiteral("Acme Ltd"));
        CHECK(d.notBefore.isValid() && d.notAfter.isValid() && d.notAfter > d.notBefore);
        CHECK(!d.sha256.isEmpty() && d.sha256.contains(u':'));
        X509 *c = parse(ca.caCertPem());
        CHECK(c != nullptr);
        if (c) {
            CHECK(commonName(X509_get_subject_name(c)) == QStringLiteral("My Custom CA"));
            char ob[128] = {0};
            const int on = X509_NAME_get_text_by_NID(X509_get_subject_name(c), NID_organizationName, ob, sizeof(ob));
            CHECK(on > 0 && QString::fromUtf8(ob, on) == QStringLiteral("Acme Ltd"));
            X509_free(c);
        }
        // A leaf still verifies against the regenerated root.
        const CertAuthority::Leaf lf = ca.leafFor(QStringLiteral("host.test"));
        X509 *lc = parse(lf.certPem), *cc = parse(ca.caCertPem());
        CHECK(lc && cc);
        if (lc && cc) {
            EVP_PKEY *pub = X509_get_pubkey(cc);
            CHECK(pub && X509_verify(lc, pub) == 1);
            if (pub) EVP_PKEY_free(pub);
        }
        if (lc) X509_free(lc);
        if (cc) X509_free(cc);
        // Querying installation status must not crash (not installed here).
        (void)ca.isInstalled();
    }

    if (savedAppData.isEmpty())
        qunsetenv("APPDATA");
    else
        qputenv("APPDATA", savedAppData);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    testProfile();
    testRules();
    testDns();
    testProxies();
    testUdpProxy();
    testWinDivertPacket();
    testCapture();
    testCaptureDatagrams();
    testCertAuthority();
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
