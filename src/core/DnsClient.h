#pragma once

#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <QTimer>
#include <functional>

class QUdpSocket;

// Asynchronous stub resolver used by the relay. It talks to the system DNS
// servers directly; the driver lets this process's traffic through untouched,
// so its queries are never answered by the fake DNS responder.
class DnsClient : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(const QList<QHostAddress> &addrs, const QString &error)>;

    explicit DnsClient(QObject *parent = nullptr);

    bool bind(quint16 port = 0);
    quint16 port() const { return m_port; }
    void resolve(const QString &hostname, Callback cb);

    static QList<QHostAddress> systemDnsServers();

private:
    struct Pending {
        QString name;
        quint16 qtype;
        int attempt = 0;
        qint64 deadline = 0;
        Callback cb;
    };
    void send(quint16 id);
    void onReadyRead();
    void onTick();

    QUdpSocket *m_socket = nullptr;
    quint16 m_port = 0;
    quint16 m_nextId = 1;
    QList<QHostAddress> m_servers;
    QHash<quint16, Pending> m_pending;
    QHash<QString, QPair<QList<QHostAddress>, qint64>> m_cache;
    QTimer m_timer;
};
