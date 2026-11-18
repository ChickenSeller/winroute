#pragma once

#include "Profile.h"

#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <functional>

class DnsClient;

// Resolves host names through a proxy or a chain: a DNS query over TCP to a
// DNS server, tunnelled like any proxied connection. Used when a rule (or the
// profile) names the proxy that resolves its host names.
class ProxyDns : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(const QList<QHostAddress> &addrs, const QString &error)>;

    // localDns resolves the proxies' own host names.
    explicit ProxyDns(DnsClient *localDns, QObject *parent = nullptr);

    // Looks up the IPv4 addresses of `name` through proxy or chain `resolverId`
    // of `profile`, asking `server` (port 53). Answers are cached per resolver.
    void resolve(const Profile &profile, int resolverId, const QString &server, const QString &name, Callback cb);

    // The ways to reach the DNS server through `id`, tried in order: one hop
    // list for a proxy or a simple chain, one per member otherwise.
    static QList<QList<ProxyServer>> routes(const Profile &profile, int id);
    // "8.8.8.8", "8.8.8.8:5353" or "[2001:db8::1]:53"; false when invalid.
    static bool parseServer(const QString &text, QHostAddress *addr, quint16 *port);
    // "proxy 1.2.3.4:1080" / "chain Name", for messages.
    static QString resolverName(const Profile &profile, int id);

private:
    struct Job {
        QList<QList<ProxyServer>> routes;
        int route = 0;
        QHostAddress server;
        quint16 port = 53;
        QString name;
        QString key;
        QString lastError;
    };
    void attempt(Job job);
    void finish(const QString &key, const QList<QHostAddress> &addrs, const QString &error, quint32 ttl);

    DnsClient *m_localDns;
    quint16 m_nextId = 1;
    QHash<QString, QList<Callback>> m_waiting;                     // key -> callbacks
    QHash<QString, QPair<QList<QHostAddress>, qint64>> m_cache;    // key -> addresses, expiry
};
