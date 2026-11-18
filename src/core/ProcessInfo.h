#pragma once

#include <QHostAddress>
#include <QList>
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

// Owner pid of a local UDP endpoint, via GetExtendedUdpTable. 0 if not found.
quint32 udpOwnerPid(const QHostAddress &localAddr, quint16 localPort);

quint32 currentSessionId();
bool isElevated();
bool terminate(quint32 pid, QString *error = nullptr);
// The process and all its descendants (children, their children, ...), from a
// single snapshot, the process itself first. For ending a whole process tree.
QList<quint32> descendants(quint32 pid);
// Asks the process to quit as if the user closed it: WM_CLOSE to each of its
// visible top-level windows. False when it has no such window.
bool requestClose(quint32 pid);
// Whether the process is still running (false once it has exited).
bool isRunning(quint32 pid);

} // namespace procinfo
