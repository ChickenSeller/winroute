#pragma once

#include "FakeDns.h"
#include "FlowTable.h"
#include "Interceptor.h"
#include "Profile.h"
#include "ProxyGateway.h"
#include "Relay.h"

#include <QObject>
#include <QThread>
#include <memory>

// Owns the proxification pipeline: interceptor (driver) -> flow table -> relay.
class Engine : public QObject {
    Q_OBJECT
public:
    explicit Engine(QObject *parent = nullptr);
    ~Engine() override;

    bool start(QString *error);
    void stop();
    bool isRunning() const { return m_running; }
    QString backendName() const;

    // The active interceptor backend: "netfilter" (default) or "windivert".
    // Persisted in QSettings under "backend/active". Switching stops the engine
    // and replaces the interceptor; the caller restarts it if it was running.
    QString backendKey() const { return m_backendKey; }
    bool setBackend(const QString &key, QString *error);

    void setProfile(const Profile &profile);
    RuleEnginePtr rules() const { return m_rules; }

    // Inbound proxy service (lets other machines reach the network through this
    // one). It runs in its own thread, independent of the capture backend, and
    // follows the current rules. Applies the given configuration: stops the
    // service, then starts it again if it is enabled. Returns false (error set)
    // when an enabled service cannot bind its port.
    bool applyGatewayConfig(const ProxyGateway::Config &config, QString *error);
    ProxyGateway *gateway() const { return m_gateway; }

    Relay *relay() const { return m_relay; }
    void closeConnection(quint64 id) { m_relay->closeConnection(id); }
    void resetProxyFailures() { m_relay->resetProxyFailures(); }
    void setLocalDnsAvailable(bool available) { m_interceptor->setLocalDnsAvailable(available); }

private:
    FlowTable m_flows;
    FakeDns m_fakeDns;
    DnsCache m_dnsCache;
    QThread m_relayThread;
    Relay *m_relay = nullptr;
    QThread m_gatewayThread;
    ProxyGateway *m_gateway = nullptr;
    std::unique_ptr<Interceptor> m_interceptor;
    QString m_backendKey;
    RuleEnginePtr m_rules;
    bool m_running = false;
};
