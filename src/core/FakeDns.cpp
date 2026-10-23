#include "FakeDns.h"
#include "Utils.h"

static constexpr quint32 kFakeBase = 0xC6120000;   // 198.18.0.0
static constexpr quint32 kFakeSize = 0x20000;      // /15

QHostAddress FakeDns::addressFor(const QString &hostname0)
{
    const QString hostname = hostname0.toLower();
    QMutexLocker lock(&m_mutex);
    auto it = m_byName.constFind(hostname);
    if (it != m_byName.cend())
        return QHostAddress(*it);
    // Skip .0 and .255 host parts; wrap around (oldest names get recycled).
    quint32 addr;
    do {
        m_next = (m_next + 1) % kFakeSize;
        addr = kFakeBase + m_next;
    } while ((addr & 0xFF) == 0 || (addr & 0xFF) == 0xFF);
    auto old = m_byAddr.constFind(addr);
    if (old != m_byAddr.cend())
        m_byName.remove(*old);
    m_byName.insert(hostname, addr);
    m_byAddr.insert(addr, hostname);
    return QHostAddress(addr);
}

QString FakeDns::hostnameFor(const QHostAddress &addr) const
{
    const QHostAddress a = util::normalize(addr);
    if (a.protocol() != QAbstractSocket::IPv4Protocol)
        return {};
    QMutexLocker lock(&m_mutex);
    return m_byAddr.value(a.toIPv4Address());
}

bool FakeDns::isFake(const QHostAddress &addr)
{
    const QHostAddress a = util::normalize(addr);
    if (a.protocol() != QAbstractSocket::IPv4Protocol)
        return false;
    return (a.toIPv4Address() & 0xFFFE0000) == kFakeBase;
}

void FakeDns::clear()
{
    QMutexLocker lock(&m_mutex);
    m_byName.clear();
    m_byAddr.clear();
}

QByteArray DnsCache::key(const QHostAddress &addr0)
{
    const QHostAddress addr = util::normalize(addr0);
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 v = addr.toIPv4Address();
        return QByteArray(reinterpret_cast<const char *>(&v), 4);
    }
    const Q_IPV6ADDR v = addr.toIPv6Address();
    return QByteArray(reinterpret_cast<const char *>(&v), 16);
}

void DnsCache::insert(const QHostAddress &addr, const QString &hostname)
{
    QMutexLocker lock(&m_mutex);
    if (m_names.size() > 65536)
        m_names.clear();
    m_names.insert(key(addr), hostname.toLower());
}

QString DnsCache::lookup(const QHostAddress &addr) const
{
    QMutexLocker lock(&m_mutex);
    return m_names.value(key(addr));
}

void DnsCache::clear()
{
    QMutexLocker lock(&m_mutex);
    m_names.clear();
}
