// Tests for UI pieces that run real network logic: the proxy checker.
// Run with QT_QPA_PLATFORM=offscreen.

#include "core/CertAuthority.h"
#include "core/Profile.h"
#include "ui/ProxyCheckerDialog.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QThread>
#include <cstdio>

static int g_checks = 0, g_failures = 0;
#define CHECK(cond)                                                                 \
    do {                                                                            \
        ++g_checks;                                                                 \
        if (!(cond)) {                                                              \
            ++g_failures;                                                           \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                           \
    } while (0)

namespace {

// A minimal SOCKS5 proxy: no authentication, CONNECT only (UDP ASSOCIATE is
// refused). Data is piped both ways; a side closing closes the other.
class Socks5 : public QTcpServer {
protected:
    void incomingConnection(qintptr fd) override
    {
        auto *c = new QTcpSocket(this);
        c->setSocketDescriptor(fd);
        auto *buf = new QByteArray;
        auto *stage = new int(0);
        connect(c, &QObject::destroyed, [buf, stage] { delete buf; delete stage; });
        connect(c, &QTcpSocket::readyRead, c, [this, c, buf, stage] {
            *buf += c->readAll();
            if (*stage == 0) {
                if (buf->size() < 2 || buf->size() < 2 + quint8(buf->at(1)))
                    return;
                buf->remove(0, 2 + quint8(buf->at(1)));
                c->write(QByteArray("\x05\x00", 2));
                *stage = 1;
            }
            if (*stage == 1) {
                if (buf->size() < 5)
                    return;
                const quint8 atyp = quint8(buf->at(3));
                if (buf->at(1) != 1 || (atyp != 1 && atyp != 3)) {   // CONNECT to IPv4 or a name
                    c->write(QByteArray("\x05\x07\x00\x01\0\0\0\0\0\0", 10));
                    c->disconnectFromHost();
                    *stage = 3;
                    return;
                }
                const int addrLen = atyp == 1 ? 4 : 1 + quint8(buf->at(4));
                if (buf->size() < 4 + addrLen + 2)
                    return;
                const QString host = atyp == 1
                    ? QHostAddress(quint32((quint8(buf->at(4)) << 24) | (quint8(buf->at(5)) << 16)
                                           | (quint8(buf->at(6)) << 8) | quint8(buf->at(7)))).toString()
                    : QString::fromLatin1(buf->mid(5, addrLen - 1));
                const quint16 port = quint16((quint8(buf->at(4 + addrLen)) << 8) | quint8(buf->at(5 + addrLen)));
                buf->remove(0, 4 + addrLen + 2);
                *stage = 2;
                auto *u = new QTcpSocket(c);
                connect(u, &QTcpSocket::connected, c, [c, u, buf] {
                    c->write(QByteArray("\x05\x00\x00\x01\x7f\x00\x00\x01\x00\x00", 10));
                    if (!buf->isEmpty())
                        u->write(*buf);
                    buf->clear();
                });
                connect(u, &QTcpSocket::readyRead, c, [c, u] { c->write(u->readAll()); });
                connect(u, &QTcpSocket::disconnected, c, [c] { c->disconnectFromHost(); });
                u->connectToHost(host, port);
                return;
            }
            if (*stage == 2) {
                auto *u = c->findChild<QTcpSocket *>();
                if (u && u->state() == QAbstractSocket::ConnectedState) {
                    u->write(*buf);
                    buf->clear();
                }
            }
        });
        connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
    }
};

// Answers one HTTP request and closes at once, as servers do for
// "Connection: close": the reply and the close reach the client together.
class HttpOnce : public QTcpServer {
public:
    bool tls = false;
    QByteArray certPem, keyPem;
protected:
    void incomingConnection(qintptr fd) override
    {
        auto *s = new QSslSocket(this);
        s->setSocketDescriptor(fd);
        auto *req = new QByteArray;
        connect(s, &QObject::destroyed, [req] { delete req; });
        connect(s, &QSslSocket::readyRead, s, [s, req] {
            *req += s->readAll();
            if (!req->contains("\r\n\r\n"))
                return;
            s->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok");
            s->disconnectFromHost();
        });
        connect(s, &QSslSocket::disconnected, s, &QObject::deleteLater);
        if (tls) {
            QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
            cfg.setLocalCertificate(QSslCertificate(certPem, QSsl::Pem));
            cfg.setPrivateKey(QSslKey(keyPem, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey));
            s->setSslConfiguration(cfg);
            s->startServerEncryption();
        }
    }
};

// Runs the checker on proxy -> 127.0.0.1:port and returns its output text.
// With busy set, the event loop pauses between rounds, as WinRoute's busy UI
// thread does: data and the peer's close then arrive together.
QString runChecker(quint16 proxyPort, quint16 port, bool busy = false)
{
    ProxyServer p;
    p.id = 100;
    p.type = ProxyType::Socks5;
    p.address = QStringLiteral("127.0.0.1");
    p.port = proxyPort;
    ProxyCheckerDialog dlg(QList<ProxyServer>{p});
    dlg.findChild<QLineEdit *>(QStringLiteral("hostEdit"))->setText(QStringLiteral("127.0.0.1"));
    dlg.findChild<QSpinBox *>(QStringLiteral("portSpin"))->setValue(port);
    auto *start = dlg.findChild<QPushButton *>(QStringLiteral("startButton"));
    auto *output = dlg.findChild<QTextEdit *>(QStringLiteral("outputEdit"));
    dlg.startTest();
    QElapsedTimer t;
    t.start();
    while (!start->isEnabled() && t.elapsed() < 30000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (busy)
            QThread::msleep(100);
    }
    return output->toPlainText();
}

void testProxyChecker()
{
    Socks5 proxy;
    CHECK(proxy.listen(QHostAddress::LocalHost, 0));

    // Plain HTTP: the reply must be read even though the server closes right
    // after sending it.
    {
        HttpOnce http;
        CHECK(http.listen(QHostAddress::LocalHost, 0));
        for (const bool busy : {false, true}) {
            const int before = g_failures;
            const QString text = runChecker(proxy.serverPort(), http.serverPort(), busy);
            CHECK(text.contains(QStringLiteral("HTTP/1.1 200 OK")));
            CHECK(text.contains(QStringLiteral("working correctly")));
            if (g_failures != before)
                std::printf("%s\n", qPrintable(text));
        }
    }

    // HTTPS (port 443): a TLS handshake, then the same request and close.
    {
        CertAuthority ca;
        CHECK(ca.ensureCa(nullptr));
        const CertAuthority::Leaf leaf = ca.leafFor(QStringLiteral("127.0.0.1"));
        HttpOnce https;
        https.tls = true;
        https.certPem = leaf.certPem;
        https.keyPem = leaf.keyPem;
        if (!https.listen(QHostAddress::LocalHost, 443)) {
            std::printf("  (skipped the HTTPS checker test: port 443 is in use)\n");
        } else {
            for (const bool busy : {false, true}) {
                const int before = g_failures;
                const QString text = runChecker(proxy.serverPort(), 443, busy);
                CHECK(text.contains(QStringLiteral("TLS handshake")));
                CHECK(text.contains(QStringLiteral("HTTP/1.1 200 OK")));
                CHECK(text.contains(QStringLiteral("working correctly")));
                if (g_failures != before)
                    std::printf("%s\n", qPrintable(text));
            }
        }
    }
}

}   // namespace

int main(int argc, char **argv)
{
    // Keep the CA the HTTPS test creates out of the real %APPDATA%.
    QTemporaryDir tmp;
    qputenv("APPDATA", QDir::toNativeSeparators(tmp.path()).toUtf8());
    QApplication app(argc, argv);
    QSslSocket::setActiveBackend(QStringLiteral("openssl"));
    testProxyChecker();
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
