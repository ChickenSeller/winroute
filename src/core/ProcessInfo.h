#pragma once

#include <QHostAddress>
#include <QString>

namespace procinfo {

struct Info {
    quint32 pid = 0;
    QString path;        // full image path (empty if inaccessible)
    QString name;        // file name, or "System" / "<pid N>"
    quint32 sessionId = 0;
    bool is64 = true;
};

// Cached lookup; safe to call from any thread.
Info lookup(quint32 pid);

// Owner pid of a local TCP endpoint, via GetExtendedTcpTable. 0 if not found.
quint32 tcpOwnerPid(const QHostAddress &localAddr, quint16 localPort);

quint32 currentSessionId();
bool isElevated();
bool terminate(quint32 pid, QString *error = nullptr);

} // namespace procinfo
