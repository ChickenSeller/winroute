#pragma once

#include "Profile.h"
#include "RuleEngine.h"

#include <QHash>
#include <QSet>
#include <QString>

// Detects the program behind a proxy that runs on this machine (Proxifier's
// "infinite connection loop" case): a local proxy client such as one listening
// on 127.0.0.1:1080 makes its own connections to its server, and if a rule
// sends those back into 127.0.0.1:1080 they go round forever. A process that
// listens on the port of one of the profile's local proxies is that program,
// so its connections are sent directly instead.
//
// Thread-safe; the listener table is read with GetExtendedTcpTable and cached
// for a moment, and only when the profile has a proxy on this machine.
namespace loopguard {

// Whether a proxy's address is this machine: loopback, "localhost", or an
// address of one of its adapters.
bool isLocalAddress(const QString &address);

// The proxy ("address:port") that this process serves, when it listens on the
// port of one of the profile's proxies on this machine; empty otherwise.
QString localProxyOf(quint32 pid, const Profile &profile);

// With loop detection on, turns a Proxy/Chain match of such a process into a
// direct connection (keeping the rule's name) and logs it once per process.
// Returns whether it did.
bool bypass(quint32 pid, const QString &appName, const Profile &profile, RuleMatch *match);

// For tests: the TCP ports each process listens on (pid -> ports).
QHash<quint32, QSet<quint16>> listeners();

} // namespace loopguard
