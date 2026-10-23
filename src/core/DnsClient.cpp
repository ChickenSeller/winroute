#include "DnsClient.h"
#include "DnsMessage.h"
#include "FakeDns.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QNetworkDatagram>
#include <QUdpSocket>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <vector>

static constexpr int kTimeoutMs = 2000;
static constexpr int kMaxAttempts = 4;

DnsClient::DnsClient(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &DnsClient::onTick);
}

bool DnsClient::bind(quint16 port)
{
    m_socket = new QUdpSocket(this);
    if (!m_socket->bind(QHostAddress::Any, port)) {
        delete m_socket;
        m_socket = nullptr;
        return false;
    }
    m_port = m_socket->localPort();
    connect(m_socket, &QUdpSocket::readyRead, this, &DnsClient::onReadyRead);
    m_timer.start();
    return true;
}

QList<QHostAddress> DnsClient::systemDnsServers()
{
    QList<QHostAddress> out;
    ULONG size = 16 * 1024;
    std::vector<char> buf(size);
    ULONG rc = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr,
                                    reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()), &size);
    if (rc == ERROR_BUFFER_OVERFLOW) {
        buf.resize(size);
        rc = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, nullptr,
                                  reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()), &size);
    }
    if (rc != NO_ERROR)
        return out;
    for (auto *a = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()); a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;
        for (auto *d = a->FirstDnsServerAddress; d; d = d->Next) {
            QHostAddress addr(d->Address.lpSockaddr);
            // fec0:0:0:ffff::1 etc. are deprecated site-local placeholders
            if (addr.isNull() || addr.toString().startsWith("fec0:", Qt::CaseInsensitive))
                continue;
            if (!out.contains(addr))
                out << addr;
        }
    }
    return out;
}

void DnsClient::resolve(const QString &hostname, Callback cb)
{
    const QHostAddress literal(hostname);
    if (!literal.isNull()) {
        cb({literal}, {});
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QString key = hostname.toLower();
    auto c = m_cache.constFind(key);
    if (c != m_cache.cend() && c->second > now) {
        cb(c->first, {});
        return;
    }
    if (!m_socket) {
        cb({}, QCoreApplication::translate("DnsClient", "resolver is not available"));
        return;
    }
    if (m_servers.isEmpty())
        m_servers = systemDnsServers();
    if (m_servers.isEmpty()) {
        cb({}, QCoreApplication::translate("DnsClient", "no DNS servers configured"));
        return;
    }
    const quint16 id = m_nextId++;
    m_pending.insert(id, Pending{key, dns::TypeA, 0, 0, std::move(cb)});
    send(id);
}

void DnsClient::send(quint16 id)
{
    auto it = m_pending.find(id);
    if (it == m_pending.end())
        return;
    const QHostAddress server = m_servers[it->attempt % m_servers.size()];
    it->deadline = QDateTime::currentMSecsSinceEpoch() + kTimeoutMs;
    m_socket->writeDatagram(dns::buildQuery(id, it->name, it->qtype), server, 53);
}

void DnsClient::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram dg = m_socket->receiveDatagram();
        dns::Question q;
        QList<dns::Answer> answers;
        if (!dns::parseAnswers(dg.data(), &q, &answers))
            continue;
        auto it = m_pending.find(q.id);
        if (it == m_pending.end() || q.name.compare(it->name, Qt::CaseInsensitive) != 0)
            continue;
        QList<QHostAddress> addrs;
        quint32 ttl = 300;
        for (const auto &a : answers) {
            if ((a.type == dns::TypeA || a.type == dns::TypeAAAA) && !FakeDns::isFake(a.address)) {
                addrs << a.address;
                ttl = qMin(ttl, qMax<quint32>(a.ttl, 10));
            }
        }
        const int rcode = q.flags & 0xF;
        if (addrs.isEmpty() && it->qtype == dns::TypeA && rcode == 0) {
            // No IPv4 address: retry for IPv6.
            it->qtype = dns::TypeAAAA;
            it->attempt = 0;
            send(q.id);
            continue;
        }
        Pending p = it.value();
        m_pending.erase(it);
        if (addrs.isEmpty()) {
            p.cb({}, rcode == 3 ? QCoreApplication::translate("DnsClient", "host not found") : QCoreApplication::translate("DnsClient", "DNS error %1").arg(rcode));
        } else {
            m_cache.insert(p.name, {addrs, QDateTime::currentMSecsSinceEpoch() + qint64(ttl) * 1000});
            p.cb(addrs, {});
        }
    }
}

void DnsClient::onTick()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<quint16> retry, failed;
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        if (it->deadline > now)
            continue;
        if (++it->attempt >= kMaxAttempts)
            failed << it.key();
        else
            retry << it.key();
    }
    for (quint16 id : retry)
        send(id);
    for (quint16 id : failed) {
        Pending p = m_pending.take(id);
        p.cb({}, QCoreApplication::translate("DnsClient", "DNS request timed out"));
    }
    if (m_cache.size() > 2048)
        m_cache.clear();
}
