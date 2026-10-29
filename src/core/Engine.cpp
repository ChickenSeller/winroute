#include "Engine.h"
#include "Logger.h"
#include "NetFilterInterceptor.h"
#include "Utils.h"
#include "WinDivertInterceptor.h"

#include <QCoreApplication>
#include <QSettings>

namespace {

// Builds the interceptor for a backend key, falling back to NetFilter when
// WinDivert is selected but its DLL/driver are not present.
std::unique_ptr<Interceptor> makeInterceptor(QString *key)
{
    if (*key == QLatin1String("windivert") && WinDivertInterceptor::available())
        return std::make_unique<WinDivertInterceptor>();
    *key = "netfilter";
    return std::make_unique<NetFilterInterceptor>();
}

} // namespace

Engine::Engine(QObject *parent)
    : QObject(parent)
{
    m_relayThread.setObjectName("relay");
    m_relay = new Relay(&m_flows);
    m_relay->moveToThread(&m_relayThread);
    connect(&m_relayThread, &QThread::finished, m_relay, &QObject::deleteLater);
    m_relayThread.start();
    m_backendKey = util::appSettings().value("backend/active", "netfilter").toString();
    m_interceptor = makeInterceptor(&m_backendKey);

    // The inbound proxy service runs in its own thread; its sockets must be
    // created there, so start()/stop() are invoked on that thread.
    m_gatewayThread.setObjectName("gateway");
    m_gateway = new ProxyGateway;
    m_gateway->moveToThread(&m_gatewayThread);
    connect(&m_gatewayThread, &QThread::finished, m_gateway, &QObject::deleteLater);
    m_gatewayThread.start();
}

bool Engine::setBackend(const QString &key, QString *error)
{
    QString wanted = key;
    if (wanted != QLatin1String("windivert"))
        wanted = "netfilter";
    if (wanted == m_backendKey)
        return true;
    if (wanted == QLatin1String("windivert") && !WinDivertInterceptor::available()) {
        if (error)
            *error = QCoreApplication::translate("Engine", "WinDivert is not available (WinDivert.dll or its driver is missing).");
        return false;
    }
    stop();
    m_interceptor = makeInterceptor(&wanted);
    m_backendKey = wanted;
    m_interceptor->setRules(m_rules);
    util::appSettings().setValue("backend/active", m_backendKey);
    return true;
}

Engine::~Engine()
{
    stop();
    // Tear the proxy service down in its own thread before stopping that thread.
    QMetaObject::invokeMethod(m_gateway, [this] { m_gateway->stop(); }, Qt::BlockingQueuedConnection);
    m_gatewayThread.quit();
    m_gatewayThread.wait();
    m_relayThread.quit();
    m_relayThread.wait();
}

QString Engine::backendName() const
{
    return m_interceptor->name();
}

bool Engine::start(QString *error)
{
    if (m_running)
        return true;
    bool listening = false;
    QString err;
    QMetaObject::invokeMethod(m_relay, [&] { listening = m_relay->port() != 0 || m_relay->listen(&err); },
                              Qt::BlockingQueuedConnection);
    if (!listening) {
        *error = err;
        return false;
    }
    InterceptorContext ctx;
    ctx.flows = &m_flows;
    ctx.fakeDns = &m_fakeDns;
    ctx.dnsCache = &m_dnsCache;
    ctx.relay = m_relay;
    ctx.relayPort = m_relay->port();
    ctx.relayIPv6 = m_relay->hasIPv6();
    m_interceptor->setRules(m_rules);
    if (!m_interceptor->start(ctx, error))
        return false;
    m_running = true;
    logVerbose(QCoreApplication::translate("Engine", "Local relay is listening on port %1").arg(ctx.relayPort));
    return true;
}

void Engine::stop()
{
    if (!m_running)
        return;
    // Close the relay first: its UDP sessions call into the driver API, which
    // the interceptor's stop() tears down.
    m_relay->closeAll();
    m_interceptor->stop();
    m_flows.clear();
    m_running = false;
}

void Engine::setProfile(const Profile &profile)
{
    m_rules = std::make_shared<const RuleEngine>(profile);
    m_relay->setRules(m_rules);
    m_interceptor->setRules(m_rules);
    m_gateway->setRules(m_rules);
}

bool Engine::applyGatewayConfig(const ProxyGateway::Config &config, QString *error)
{
    m_gateway->setRules(m_rules);   // the service forwards through the current rules
    bool ok = true;
    QString err;
    QMetaObject::invokeMethod(
        m_gateway,
        [&] {
            m_gateway->stop();
            if (config.enabled)
                ok = m_gateway->start(config, &err);
        },
        Qt::BlockingQueuedConnection);
    if (!ok && error)
        *error = err;
    return ok;
}
