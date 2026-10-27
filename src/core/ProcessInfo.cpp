#include "ProcessInfo.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QMutex>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <vector>

namespace procinfo {

namespace {
struct CacheEntry {
    Info info;
    qint64 expires;
};
QMutex g_mutex;
QHash<quint32, CacheEntry> g_cache;
} // namespace

static Info query(quint32 pid)
{
    Info info;
    info.pid = pid;
    if (pid == 0) {
        info.name = "Idle";
        return info;
    }
    if (pid == 4) {
        info.name = "System";
        return info;
    }
    DWORD session = 0;
    if (ProcessIdToSessionId(pid, &session))
        info.sessionId = session;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h) {
        wchar_t buf[MAX_PATH * 2];
        DWORD len = DWORD(std::size(buf));
        if (QueryFullProcessImageNameW(h, 0, buf, &len))
            info.path = QString::fromWCharArray(buf, int(len));
        BOOL wow = FALSE;
        if (IsWow64Process(h, &wow))
            info.is64 = !wow;
        CloseHandle(h);
    }
    info.name = info.path.isEmpty() ? QString("<pid %1>").arg(pid) : QFileInfo(info.path).fileName();
    return info;
}

Info lookup(quint32 pid)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    {
        QMutexLocker lock(&g_mutex);
        auto it = g_cache.constFind(pid);
        if (it != g_cache.cend() && it->expires > now)
            return it->info;
    }
    // Pids get reused, so entries expire quickly; the lookup itself is cheap.
    Info info = query(pid);
    QMutexLocker lock(&g_mutex);
    if (g_cache.size() > 4096)
        g_cache.clear();
    g_cache.insert(pid, {info, now + 5000});
    return info;
}

quint32 tcpOwnerPid(const QHostAddress &localAddr, quint16 localPort)
{
    const bool v6 = localAddr.protocol() == QAbstractSocket::IPv6Protocol;
    const ULONG af = v6 ? AF_INET6 : AF_INET;
    ULONG size = 0;
    GetExtendedTcpTable(nullptr, &size, FALSE, af, TCP_TABLE_OWNER_PID_ALL, 0);
    std::vector<char> buf;
    DWORD rc;
    do {
        buf.resize(size + 4096);
        size = ULONG(buf.size());
        rc = GetExtendedTcpTable(buf.data(), &size, FALSE, af, TCP_TABLE_OWNER_PID_ALL, 0);
    } while (rc == ERROR_INSUFFICIENT_BUFFER);
    if (rc != NO_ERROR)
        return 0;

    const u_short portN = htons(localPort);
    quint32 wildcardMatch = 0;
    if (!v6) {
        auto *t = reinterpret_cast<MIB_TCPTABLE_OWNER_PID *>(buf.data());
        const DWORD addrN = htonl(localAddr.toIPv4Address());
        for (DWORD i = 0; i < t->dwNumEntries; ++i) {
            const auto &r = t->table[i];
            if (u_short(r.dwLocalPort) != portN || r.dwState == MIB_TCP_STATE_LISTEN)
                continue;
            if (r.dwLocalAddr == addrN)
                return r.dwOwningPid;
            if (r.dwLocalAddr == 0)
                wildcardMatch = r.dwOwningPid;
        }
    } else {
        auto *t = reinterpret_cast<MIB_TCP6TABLE_OWNER_PID *>(buf.data());
        const Q_IPV6ADDR a = localAddr.toIPv6Address();
        static const UCHAR zero[16] = {};
        for (DWORD i = 0; i < t->dwNumEntries; ++i) {
            const auto &r = t->table[i];
            if (u_short(r.dwLocalPort) != portN || r.dwState == MIB_TCP_STATE_LISTEN)
                continue;
            if (memcmp(r.ucLocalAddr, &a, 16) == 0)
                return r.dwOwningPid;
            if (memcmp(r.ucLocalAddr, zero, 16) == 0)
                wildcardMatch = r.dwOwningPid;
        }
    }
    return wildcardMatch;
}

quint32 udpOwnerPid(const QHostAddress &localAddr, quint16 localPort)
{
    const bool v6 = localAddr.protocol() == QAbstractSocket::IPv6Protocol;
    const ULONG af = v6 ? AF_INET6 : AF_INET;
    ULONG size = 0;
    GetExtendedUdpTable(nullptr, &size, FALSE, af, UDP_TABLE_OWNER_PID, 0);
    std::vector<char> buf;
    DWORD rc;
    do {
        buf.resize(size + 4096);
        size = ULONG(buf.size());
        rc = GetExtendedUdpTable(buf.data(), &size, FALSE, af, UDP_TABLE_OWNER_PID, 0);
    } while (rc == ERROR_INSUFFICIENT_BUFFER);
    if (rc != NO_ERROR)
        return 0;

    const u_short portN = htons(localPort);
    quint32 wildcardMatch = 0;
    if (!v6) {
        auto *t = reinterpret_cast<MIB_UDPTABLE_OWNER_PID *>(buf.data());
        const DWORD addrN = htonl(localAddr.toIPv4Address());
        for (DWORD i = 0; i < t->dwNumEntries; ++i) {
            const auto &r = t->table[i];
            if (u_short(r.dwLocalPort) != portN)
                continue;
            if (r.dwLocalAddr == addrN)
                return r.dwOwningPid;
            if (r.dwLocalAddr == 0)
                wildcardMatch = r.dwOwningPid;
        }
    } else {
        auto *t = reinterpret_cast<MIB_UDP6TABLE_OWNER_PID *>(buf.data());
        const Q_IPV6ADDR a = localAddr.toIPv6Address();
        static const UCHAR zero[16] = {};
        for (DWORD i = 0; i < t->dwNumEntries; ++i) {
            const auto &r = t->table[i];
            if (u_short(r.dwLocalPort) != portN)
                continue;
            if (memcmp(r.ucLocalAddr, &a, 16) == 0)
                return r.dwOwningPid;
            if (memcmp(r.ucLocalAddr, zero, 16) == 0)
                wildcardMatch = r.dwOwningPid;
        }
    }
    return wildcardMatch;
}

quint32 currentSessionId()
{
    DWORD s = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &s);
    return s;
}

bool isElevated()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    TOKEN_ELEVATION e{};
    DWORD len = 0;
    const bool ok = GetTokenInformation(token, TokenElevation, &e, sizeof(e), &len) && e.TokenIsElevated;
    CloseHandle(token);
    return ok;
}

bool terminate(quint32 pid, QString *error)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h || !TerminateProcess(h, 1)) {
        if (error)
            *error = QString("error %1").arg(GetLastError());
        if (h)
            CloseHandle(h);
        return false;
    }
    CloseHandle(h);
    return true;
}

} // namespace procinfo
