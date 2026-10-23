#pragma once

#include <QHash>
#include <QHostAddress>
#include <QMutex>
#include <QString>

// Fake address pool for "Resolve hostnames through proxy". Addresses come from
// 198.18.0.0/15 (RFC 2544 benchmarking range) rather than Proxifier's 127.8.*.*,
// because loopback traffic cannot be redirected at the network layer.
class FakeDns {
public:
    QHostAddress addressFor(const QString &hostname);
    QString hostnameFor(const QHostAddress &addr) const;
    static bool isFake(const QHostAddress &addr);
    void clear();

private:
    mutable QMutex m_mutex;
    QHash<QString, quint32> m_byName;
    QHash<quint32, QString> m_byAddr;
    quint32 m_next = 0;
};

// IP -> host name cache filled by sniffing DNS responses, so rules with host
// name patterns work and the UI can show names instead of bare addresses.
class DnsCache {
public:
    void insert(const QHostAddress &addr, const QString &hostname);
    QString lookup(const QHostAddress &addr) const;
    void clear();

private:
    static QByteArray key(const QHostAddress &addr);
    mutable QMutex m_mutex;
    QHash<QByteArray, QString> m_names;
};
