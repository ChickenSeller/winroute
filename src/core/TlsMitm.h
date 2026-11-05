#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>

class CertAuthority;
class QSslSocket;
class QTimer;

// The two TLS handshakes of a decrypted (HTTPS decryption) connection, shared
// by the relay and the proxy gateway:
//
//  - client side: peeks at the ClientHello for the server name (SNI), then
//    presents a leaf certificate for that name, signed by WinRoute's CA. The
//    host name known for the address can be another site on the same IP (a
//    CDN, or www. and down. of one site); the SNI is what the client checks.
//  - upstream side: a TLS client to the real server, asking for and verifying
//    the same name against the system roots.
//
// Both sockets must still be in unencrypted mode with nothing read from them
// yet, so the SSL engines see the handshakes. Once both sides are encrypted,
// ready() is called and the owner pumps plaintext between them; on any error,
// failed() is called once and nothing more happens.
class TlsMitm : public QObject {
public:
    TlsMitm(QSslSocket *client, CertAuthority *ca, const QString &knownName, QObject *parent);

    std::function<void()> onReady;
    std::function<void(const QString &error)> onFailed;
    // The name the certificate is made for, once chosen, when it differs from
    // the known one (for the log).
    std::function<void(const QString &name)> onNameChosen;

    // Starts reading the ClientHello; the client handshake follows.
    void startClient();
    // The tunnel to the server is up: starts the upstream handshake as soon
    // as the name is known.
    void startUpstream(QSslSocket *upstream);

    QString name() const { return m_name; }
    bool isReady() const { return m_ready; }

private:
    void peekHello();
    void helloDone(const QString &sni);
    void startClientTls();
    void startUpstreamTls();
    void checkReady();
    void fail(const QString &error);
    static bool validHostName(const QString &name);

    QPointer<QSslSocket> m_client;
    QPointer<QSslSocket> m_upstream;
    CertAuthority *m_ca;
    QString m_known;
    QString m_name;
    QMetaObject::Connection m_helloConn;
    QPointer<QTimer> m_helloTimer;
    bool m_helloDone = false;
    bool m_upstreamPending = false;
    bool m_clientTls = false;
    bool m_upstreamTls = false;
    bool m_ready = false;
    bool m_failed = false;
};
