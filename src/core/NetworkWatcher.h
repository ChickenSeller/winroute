#pragma once

#include <QObject>
#include <QTimer>

// Emits changed() when the machine's network changes: an adapter comes up or
// goes down, or its IP addresses change (a new Wi-Fi network, a VPN, DHCP).
// Bursts of system notifications are merged into one signal, sent once the
// network has been quiet for a moment.
class NetworkWatcher : public QObject {
    Q_OBJECT
public:
    explicit NetworkWatcher(QObject *parent = nullptr);
    ~NetworkWatcher() override;

signals:
    void changed();

private:
    Q_INVOKABLE void notify();   // queued from the system's notification thread

    void *m_interfaceHandle = nullptr;
    void *m_addressHandle = nullptr;
    QTimer m_settle;
};
