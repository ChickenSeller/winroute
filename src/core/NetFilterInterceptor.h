#pragma once

#include "Interceptor.h"
#include "NetFilterApi.h"

#include <QMutex>
#include <QSet>
#include <atomic>

// Interceptor backed by the NetFilter SDK WFP driver (netfilter2.sys).
// Outgoing TCP connect requests are indicated to tcpConnectRequest(), where
// the remote address is rewritten to the local relay for proxied connections.
// UDP port 53 traffic is filtered for the fake DNS responder and to learn
// IP -> host name mappings from DNS responses.
class NetFilterInterceptor : public Interceptor {
public:
    static constexpr const char *kDriverName = "netfilter2";

    NetFilterInterceptor();
    ~NetFilterInterceptor() override;

    QString name() const override { return "NetFilter SDK WFP driver"; }
    bool start(const InterceptorContext &ctx, QString *error) override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    void setRules(RuleEnginePtr rules) override;
    void setLocalDnsAvailable(bool available) override { m_localDnsAvailable = available; }

    // Removes the driver service (used by the uninstall command line switch).
    static bool uninstallDriver(QString *error);

    // NF_EventHandler callbacks, dispatched from nfapi worker threads.
    void onTcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO info);
    void onUdpSend(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt);
    void onUdpReceive(ENDPOINT_ID id, const unsigned char *remote, const char *buf, int len, PNF_UDP_OPTIONS opt);

private:
    bool ensureDriver(QString *error);
    bool installRules(QString *error);
    RuleEnginePtr rules() const;
    bool resolveViaProxy(const ProfileOptions &opt) const;
    bool shouldFakeResolve(const RuleEngine &rules, const QString &name) const;
    QString processPath(quint32 pid, const QString &fallback) const;

    const NetFilterApi *m_api = nullptr;
    InterceptorContext m_ctx;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_localDnsAvailable{true};
    quint32 m_selfPid = 0;
    quint32 m_selfSession = 0;

    mutable QMutex m_rulesMutex;
    RuleEnginePtr m_rules;
    QSet<QString> m_proxyHosts;        // proxy host names (never faked)
    QSet<QString> m_proxyEndpoints;    // "ip:port" of proxies (loop detection)
};
