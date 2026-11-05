#include "TlsMitm.h"
#include "CertAuthority.h"
#include "TlsHello.h"

#include <QCoreApplication>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTimer>

static constexpr int kHelloWaitMs = 5000;
static constexpr qint64 kMaxHelloPeek = 64 * 1024;

TlsMitm::TlsMitm(QSslSocket *client, CertAuthority *ca, const QString &knownName, QObject *parent)
    : QObject(parent), m_client(client), m_ca(ca), m_known(knownName)
{
}

void TlsMitm::startClient()
{
    if (!m_client || !m_ca) {
        fail(QCoreApplication::translate("Relay", "HTTPS decryption is not available"));
        return;
    }
    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    // A client that sends nothing gets the known name.
    connect(timer, &QTimer::timeout, this, [this] { helloDone(QString()); });
    timer->start(kHelloWaitMs);
    m_helloTimer = timer;
    m_helloConn = connect(m_client, &QSslSocket::readyRead, this, [this] { peekHello(); });
    peekHello();
}

void TlsMitm::peekHello()
{
    if (m_helloDone || !m_client)
        return;
    // QSslSocket::peek() in unencrypted mode reads from the underlying socket
    // without consuming, so startServerEncryption() still sees all of it.
    const QByteArray head = m_client->peek(kMaxHelloPeek);
    QString sni;
    const tlshello::Result r = tlshello::serverName(head, &sni);
    if (r == tlshello::Result::NeedMore && head.size() < kMaxHelloPeek)
        return;
    helloDone(r == tlshello::Result::Done ? sni : QString());
}

void TlsMitm::helloDone(const QString &sni)
{
    if (m_helloDone || m_failed)
        return;
    m_helloDone = true;
    QObject::disconnect(m_helloConn);
    if (m_helloTimer)
        m_helloTimer->deleteLater();
    m_name = validHostName(sni) ? sni : m_known;
    if (m_name != m_known && onNameChosen)
        onNameChosen(m_name);
    startClientTls();
    if (m_upstreamPending && !m_failed) {
        m_upstreamPending = false;
        startUpstreamTls();
    }
}

// A DNS name fit for a certificate: letters, digits, '-', '_', '.'.
bool TlsMitm::validHostName(const QString &name)
{
    if (name.isEmpty() || name.size() > 253 || name.startsWith(u'.') || name.endsWith(u'.'))
        return false;
    for (const QChar c : name)
        if (!(c.isLetterOrNumber() && c.unicode() < 128) && c != u'-' && c != u'_' && c != u'.')
            return false;
    return true;
}

// Presents our leaf certificate and begins the server handshake with the client.
void TlsMitm::startClientTls()
{
    if (!m_client)
        return;
    const CertAuthority::Leaf leaf = m_ca->leafFor(m_name);
    const QSslCertificate cert(leaf.certPem, QSsl::Pem);
    const QSslKey key(leaf.keyPem, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    if (cert.isNull() || key.isNull()) {
        fail(QCoreApplication::translate("Relay", "could not create a certificate for %1").arg(m_name));
        return;
    }
    QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
    cfg.setLocalCertificate(cert);
    cfg.setPrivateKey(key);
    cfg.setAllowedNextProtocols({QByteArrayLiteral("http/1.1")});
    m_client->setSslConfiguration(cfg);
    connect(m_client, &QSslSocket::encrypted, this, [this] {
        m_clientTls = true;
        checkReady();
    });
    m_client->startServerEncryption();
}

void TlsMitm::startUpstream(QSslSocket *upstream)
{
    m_upstream = upstream;
    if (!m_upstream) {
        fail(QCoreApplication::translate("Relay", "HTTPS decryption is not available"));
        return;
    }
    // The server is asked for, and checked against, the client's name.
    if (!m_helloDone) {
        m_upstreamPending = true;
        return;
    }
    startUpstreamTls();
}

// Begins the TLS client handshake to the real server, verifying its certificate
// against the system roots (a server that is itself intercepted is refused).
void TlsMitm::startUpstreamTls()
{
    if (!m_upstream)
        return;
    m_upstream->setPeerVerifyName(m_name);
    QSslConfiguration cfg = m_upstream->sslConfiguration();
    cfg.setAllowedNextProtocols({QByteArrayLiteral("http/1.1")});
    m_upstream->setSslConfiguration(cfg);
    connect(m_upstream, &QSslSocket::encrypted, this, [this] {
        m_upstreamTls = true;
        checkReady();
    });
    connect(m_upstream, &QSslSocket::sslErrors, this, [this](const QList<QSslError> &errs) {
        QStringList msgs;
        for (const QSslError &e : errs)
            msgs << e.errorString();
        fail(QCoreApplication::translate("Relay", "the server's certificate was rejected: %1").arg(msgs.join(QStringLiteral("; "))));
    });
    m_upstream->startClientEncryption();
}

void TlsMitm::checkReady()
{
    if (m_ready || m_failed || !m_clientTls || !m_upstreamTls)
        return;
    m_ready = true;
    if (onReady)
        onReady();
}

void TlsMitm::fail(const QString &error)
{
    if (m_failed)
        return;
    m_failed = true;
    if (onFailed)
        onFailed(error);
}
