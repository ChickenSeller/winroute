#pragma once

#include "Profile.h"

#include <QHostAddress>
#include <memory>
#include <vector>

struct ConnectionRequest {
    QString appPath;      // full image path, may be empty for system processes
    QString appName;      // file name, e.g. "chrome.exe"
    quint32 pid = 0;
    QHostAddress ip;      // destination address (real, not fake)
    QString hostname;     // destination host name if known
    quint16 port = 0;
};

struct RuleMatch {
    int ruleIndex = -1;
    QString ruleName;
    RuleAction action;
};

// A profile compiled for fast matching. Immutable once built, so it can be
// shared between the packet thread and the relay thread.
class RuleEngine {
public:
    explicit RuleEngine(const Profile &profile);

    RuleMatch match(const ConnectionRequest &req) const;

    const Profile &profile() const { return m_profile; }

    // Exposed for tests and for the DNS exclusion list.
    static bool matchHostList(const QStringList &patterns, const QString &host, const QHostAddress &ip);
    static bool matchPortList(const QString &ports, quint16 port);
    static bool matchAppList(const QStringList &patterns, const QString &path, const QString &name);

private:
    struct Compiled {
        bool enabled;
        QStringList apps;
        QStringList targets;
        QString ports;
        RuleMatch result;
    };
    Profile m_profile;
    std::vector<Compiled> m_rules;
};

using RuleEnginePtr = std::shared_ptr<const RuleEngine>;
