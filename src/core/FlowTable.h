#pragma once

#include "RuleEngine.h"

#include <QHash>
#include <QHostAddress>
#include <QMutex>
#include <optional>

// A connection that the interceptor redirected to the local relay.
struct RedirectInfo {
    quint64 connId = 0;       // driver endpoint id
    quint32 pid = 0;
    QString appPath;
    QString appName;
    QHostAddress srcAddr;     // application side (local) endpoint
    quint16 srcPort = 0;
    QHostAddress dstAddr;     // original destination as seen by the application
    quint16 dstPort = 0;
    QString hostname;         // known target host name (DNS cache or fake DNS)
    bool fakeIp = false;      // dstAddr is a fake DNS address; the real IP is unknown
    RuleMatch match;
    qint64 createdMs = 0;
};

// Hands redirect metadata from the interceptor to the relay. The relay sees an
// accepted socket whose peer port is the application's local port.
class FlowTable {
public:
    void add(const RedirectInfo &info);
    std::optional<RedirectInfo> take(quint16 appLocalPort);
    void expire(qint64 olderThanMs);
    void clear();

private:
    QMutex m_mutex;
    QHash<quint16, RedirectInfo> m_pending;
};
