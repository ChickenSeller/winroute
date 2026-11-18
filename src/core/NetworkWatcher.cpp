#include "NetworkWatcher.h"

#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>

namespace {
constexpr int kSettleMs = 2000;   // adapters report several changes in a row

void WINAPI onInterfaceChange(PVOID context, PMIB_IPINTERFACE_ROW, MIB_NOTIFICATION_TYPE)
{
    QMetaObject::invokeMethod(static_cast<QObject *>(context), "notify", Qt::QueuedConnection);
}

void WINAPI onAddressChange(PVOID context, PMIB_UNICASTIPADDRESS_ROW, MIB_NOTIFICATION_TYPE)
{
    QMetaObject::invokeMethod(static_cast<QObject *>(context), "notify", Qt::QueuedConnection);
}
} // namespace

NetworkWatcher::NetworkWatcher(QObject *parent)
    : QObject(parent)
{
    m_settle.setSingleShot(true);
    m_settle.setInterval(kSettleMs);
    connect(&m_settle, &QTimer::timeout, this, &NetworkWatcher::changed);
    HANDLE h = nullptr;
    if (NotifyIpInterfaceChange(AF_UNSPEC, onInterfaceChange, this, FALSE, &h) == NO_ERROR)
        m_interfaceHandle = h;
    h = nullptr;
    if (NotifyUnicastIpAddressChange(AF_UNSPEC, onAddressChange, this, FALSE, &h) == NO_ERROR)
        m_addressHandle = h;
}

NetworkWatcher::~NetworkWatcher()
{
    // Waits for running callbacks, so none reaches a deleted watcher.
    if (m_interfaceHandle)
        CancelMibChangeNotify2(HANDLE(m_interfaceHandle));
    if (m_addressHandle)
        CancelMibChangeNotify2(HANDLE(m_addressHandle));
}

void NetworkWatcher::notify()
{
    m_settle.start();   // restarts: signal once things are quiet
}
