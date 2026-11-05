#include "NetInterfaces.h"

#include <QDateTime>
#include <QMutex>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <vector>

namespace netif {

static constexpr qint64 kCacheMs = 2000;

static QList<Adapter> readAdapters()
{
    QList<Adapter> out;
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 32 * 1024;
    std::vector<char> buf(size);
    ULONG rc = ERROR_BUFFER_OVERFLOW;
    for (int i = 0; i < 3 && rc == ERROR_BUFFER_OVERFLOW; ++i) {
        buf.resize(size);
        rc = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()), &size);
    }
    if (rc != NO_ERROR)
        return out;
    for (auto *a = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()); a; a = a->Next) {
        Adapter ad;
        ad.id = QString::fromLatin1(a->AdapterName);
        ad.name = QString::fromWCharArray(a->FriendlyName);
        ad.description = QString::fromWCharArray(a->Description);
        ad.loopback = a->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
        QList<QHostAddress> preferred, other;
        for (auto *u = a->FirstUnicastAddress; u; u = u->Next) {
            const QHostAddress addr(u->Address.lpSockaddr);
            if (addr.isNull())
                continue;
            // Tentative or duplicate addresses cannot be used yet.
            if (u->DadState != IpDadStatePreferred && u->DadState != IpDadStateDeprecated)
                continue;
            (u->DadState == IpDadStatePreferred ? preferred : other) << addr;
        }
        ad.addresses = preferred + other;
        ad.up = a->OperStatus == IfOperStatusUp && !ad.addresses.isEmpty();
        out << ad;
    }
    return out;
}

QList<Adapter> adapters()
{
    static QMutex mutex;
    static QList<Adapter> cache;
    static qint64 readAt = 0;
    QMutexLocker lock(&mutex);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (readAt == 0 || now - readAt > kCacheMs || now < readAt) {
        cache = readAdapters();
        readAt = now;
    }
    return cache;
}

Adapter find(const QString &id)
{
    if (id.isEmpty())
        return {};
    const QList<Adapter> all = adapters();
    for (const Adapter &a : all)
        if (a.id.compare(id, Qt::CaseInsensitive) == 0)
            return a;
    return {};
}

bool isUp(const QString &id)
{
    return find(id).up;
}

QHostAddress sourceAddress(const QString &id, QAbstractSocket::NetworkLayerProtocol protocol)
{
    const Adapter a = find(id);
    if (!a.up)
        return {};
    QHostAddress linkLocal;
    for (const QHostAddress &addr : a.addresses) {
        if (addr.protocol() != protocol)
            continue;
        if (addr.isLinkLocal()) {
            if (linkLocal.isNull())
                linkLocal = addr;
            continue;
        }
        return addr;
    }
    // A link-local source reaches only the local link, but it is what the
    // adapter has (e.g. an IPv4 169.254.x.x address without DHCP).
    return linkLocal;
}

bool hasProtocol(const QString &id, QAbstractSocket::NetworkLayerProtocol protocol)
{
    return !sourceAddress(id, protocol).isNull();
}

QString displayName(const QString &id)
{
    const Adapter a = find(id);
    return a.id.isEmpty() ? id : a.name;
}

} // namespace netif
