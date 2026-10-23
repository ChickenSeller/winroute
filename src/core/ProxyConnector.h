#pragma once

#include "Profile.h"

#include <QHostAddress>
#include <QObject>
#include <QPointer>
#include <QTimer>

class QTcpSocket;
class DnsClient;

struct Endpoint {
    QString host;         // host name, may be empty
    QHostAddress ip;      // address, may be null when only the name is known
    quint16 port = 0;

    QString toString() const;
};

// Establishes a TCP tunnel to a target through zero or more proxies
// (an empty hop list means a direct connection). Supports SOCKS4/4A,
// SOCKS5 (with username/password) and HTTPS CONNECT (with Basic auth).
class ProxyConnector : public QObject {
    Q_OBJECT
public:
    ProxyConnector(QTcpSocket *socket, const QList<ProxyServer> &hops, const Endpoint &target,
                   DnsClient *dns, int timeoutMs, QObject *parent = nullptr);

    void start();
    int currentHop() const { return m_hop; }

signals:
    // `leftover` holds bytes the last proxy sent after its handshake reply.
    void connected(const QByteArray &leftover);
    void failed(const QString &error);
    void progress(const QString &message);

private:
    enum class Stage { Resolving, Connecting, Greeting, Auth, Request, Done };

    void resolveNext(int index);
    void connectFirst();
    void beginHop();
    void onConnected();
    void onReadyRead();
    void onError();
    void fail(const QString &error);
    void hopDone();
    Endpoint nextEndpoint() const;
    bool hopAcceptsName(int hop) const;

    void sendSocks5Greeting();
    void sendSocks5Auth();
    void sendSocks5Connect();
    void sendSocks4Connect();
    void sendHttpConnect();
    bool handleSocks5();
    bool handleSocks4();
    bool handleHttp();
    QString proxyName(int hop) const;

    QPointer<QTcpSocket> m_socket;
    QList<ProxyServer> m_hops;
    QList<Endpoint> m_endpoints;   // m_endpoints[0] = first proxy (or target), then each next hop
    DnsClient *m_dns;
    int m_hop = 0;
    Stage m_stage = Stage::Resolving;
    QByteArray m_buf;
    QTimer m_timer;
    bool m_finished = false;
};
