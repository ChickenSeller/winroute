#include "Engine.h"
#include "Logger.h"
#include "NetFilterInterceptor.h"

#include <QCoreApplication>

Engine::Engine(QObject *parent)
    : QObject(parent)
{
    m_relayThread.setObjectName("relay");
    m_relay = new Relay(&m_flows);
    m_relay->moveToThread(&m_relayThread);
    connect(&m_relayThread, &QThread::finished, m_relay, &QObject::deleteLater);
    m_relayThread.start();
    m_interceptor = std::make_unique<NetFilterInterceptor>();
}

Engine::~Engine()
{
    stop();
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
}
