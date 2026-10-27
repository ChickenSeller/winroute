// Self-contained tests for the core library: profile IO, rule matching,
// DNS messages and the proxy protocol handshakes (against in-process proxies).

#include "core/DnsMessage.h"
#include "core/FakeDns.h"
#include "core/FlowTable.h"
#include "core/Profile.h"
#include "core/ProxyConnector.h"
#include "core/Relay.h"
#include "core/RuleEngine.h"
#include "core/UdpRelaySession.h"
#include "core/Utils.h"
#include "core/WinDivertPacket.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QtEndian>
#include <cstdio>
#include <cstring>

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

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    testProfile();
    testRules();
    testDns();
    testProxies();
    testUdpProxy();
    testWinDivertPacket();
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
