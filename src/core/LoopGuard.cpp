#include "LoopGuard.h"
#include "Logger.h"
#include "NetInterfaces.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QMutex>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <vector>

namespace loopguard {

static constexpr qint64 kCacheMs = 1000;

static QHash<quint32, QSet<quint16>> readListeners()
{
    QHash<quint32, QSet<quint16>> out;
    for (ULONG af : {ULONG(AF_INET), ULONG(AF_INET6)}) {
        DWORD size = 0;
        GetExtendedTcpTable(nullptr, &size, FALSE, af, TCP_TABLE_OWNER_PID_LISTENER, 0);
        std::vector<char> buf;
        DWORD rc = ERROR_INSUFFICIENT_BUFFER;
        for (int i = 0; i < 3 && rc == ERROR_INSUFFICIENT_BUFFER; ++i) {
            size += 4096;   // the table can grow between the two calls
            buf.resize(size);
            rc = GetExtendedTcpTable(buf.data(), &size, FALSE, af, TCP_TABLE_OWNER_PID_LISTENER, 0);
        }
        if (rc != NO_ERROR)
            continue;
        if (af == AF_INET) {
            const auto *t = reinterpret_cast<const MIB_TCPTABLE_OWNER_PID *>(buf.data());
            for (DWORD i = 0; i < t->dwNumEntries; ++i)
                out[t->table[i].dwOwningPid].insert(ntohs(u_short(t->table[i].dwLocalPort)));
        } else {
            const auto *t = reinterpret_cast<const MIB_TCP6TABLE_OWNER_PID *>(buf.data());
            for (DWORD i = 0; i < t->dwNumEntries; ++i)
                out[t->table[i].dwOwningPid].insert(ntohs(u_short(t->table[i].dwLocalPort)));
        }
    }
    return out;
}

QHash<quint32, QSet<quint16>> listeners()
{
    static QMutex mutex;
    static QHash<quint32, QSet<quint16>> cache;
    static qint64 readAt = 0;
    QMutexLocker lock(&mutex);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (readAt == 0 || now - readAt > kCacheMs || now < readAt) {
        cache = readListeners();
        readAt = now;
    }
    return cache;
}

bool isLocalAddress(const QString &address)
{
    if (address.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0)
        return true;
    QHostAddress a(address);
    if (a.isNull())
        return false;   // another host name: not resolved here
    bool ok = false;
    const QHostAddress v4(a.toIPv4Address(&ok));
    if (ok)
        a = v4;
    if (a.isLoopback())
        return true;
    a.setScopeId(QString());
    for (const netif::Adapter &ad : netif::adapters())
        for (QHostAddress own : ad.addresses) {
            own.setScopeId(QString());
            if (own == a)
                return true;
        }
    return false;
}

QString localProxyOf(quint32 pid, const Profile &profile)
{
    if (pid == 0 || pid == 4)
        return {};
    // Most profiles have no proxy on this machine: then nothing is read.
    QList<const ProxyServer *> local;
    for (const ProxyServer &p : profile.proxies)
        if (isLocalAddress(p.address))
            local << &p;
    if (local.isEmpty())
        return {};
    const QHash<quint32, QSet<quint16>> all = listeners();
    const auto it = all.constFind(pid);
    if (it == all.constEnd())
        return {};
    for (const ProxyServer *p : local)
        if (it->contains(p->port))
            return QStringLiteral("%1:%2").arg(p->address).arg(p->port);
    return {};
}

bool bypass(quint32 pid, const QString &appName, const Profile &profile, RuleMatch *match)
{
    if (!profile.options.loopDetection)
        return false;
    if (match->action.type != ActionType::Proxy && match->action.type != ActionType::Chain)
        return false;
    const QString proxy = localProxyOf(pid, profile);
    if (proxy.isEmpty())
        return false;
    match->action = RuleAction{ActionType::Direct, 0};
    // Said once per process, not for each of its connections.
    static QMutex mutex;
    static QSet<quint32> told;
    QMutexLocker lock(&mutex);
    if (!told.contains(pid)) {
        told.insert(pid);
        lock.unlock();
        logNormal(QCoreApplication::translate("Interceptor", "%1 (PID %2) is the proxy server at %3; its connections go directly to prevent a connection loop")
                      .arg(appName)
                      .arg(pid)
                      .arg(proxy));
    }
    return true;
}

} // namespace loopguard
