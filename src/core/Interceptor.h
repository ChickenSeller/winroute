#pragma once

#include "RuleEngine.h"

#include <QString>

class FlowTable;
class FakeDns;
class DnsCache;

struct InterceptorContext {
    FlowTable *flows = nullptr;
    FakeDns *fakeDns = nullptr;
    DnsCache *dnsCache = nullptr;
    quint16 relayPort = 0;
    bool relayIPv6 = false;   // the relay also accepts IPv6 connections
};

// Captures outgoing connections and redirects the ones that need a proxy to
// the local relay. The current backend is the NetFilter SDK WFP driver.
class Interceptor {
public:
    virtual ~Interceptor() = default;

    virtual QString name() const = 0;
    virtual bool start(const InterceptorContext &ctx, QString *error) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    // Called whenever the active profile changes. Thread-safe.
    virtual void setRules(RuleEnginePtr rules) = 0;

    // Result of the local DNS probe, used when "Detect DNS settings automatically" is on:
    // names are resolved through the proxy only while local DNS is unavailable.
    virtual void setLocalDnsAvailable(bool available) = 0;
};
