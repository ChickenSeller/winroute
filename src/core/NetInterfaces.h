#pragma once

#include <QAbstractSocket>
#include <QHostAddress>
#include <QList>
#include <QString>

// The machine's network adapters, for rules bound to one (Proxifier's rule
// "Advanced" > network interface). An adapter is identified by its GUID
// ("{...}", the adapter name Windows keeps across reboots and renames).
//
// The list is read with GetAdaptersAddresses and cached for a short time, so
// asking per connection is cheap. Thread-safe.
namespace netif {

struct Adapter {
    QString id;            // "{GUID}"
    QString name;          // friendly name, e.g. "Ethernet 2", "WLAN"
    QString description;   // driver description, e.g. "Intel(R) Wi-Fi 6 AX201"
    bool up = false;       // operationally up with at least one usable address
    bool loopback = false;
    QList<QHostAddress> addresses;   // unicast, preferred first
};

QList<Adapter> adapters();
// The adapter with this id, or one with an empty id if there is none.
Adapter find(const QString &id);
bool isUp(const QString &id);
// The address to bind an outgoing socket to so it leaves through the adapter:
// one of `protocol` (IPv4 or IPv6), preferring a global over a link-local
// address. Null if the adapter is down or has no such address.
QHostAddress sourceAddress(const QString &id, QAbstractSocket::NetworkLayerProtocol protocol);
// Whether the adapter currently has an address of this protocol.
bool hasProtocol(const QString &id, QAbstractSocket::NetworkLayerProtocol protocol);
// "Ethernet 2" for display, or the id when the adapter is not present.
QString displayName(const QString &id);
// The name of the adapter that owns this local address: which adapter a
// connected socket actually uses (its localAddress()). Falls back to the
// address itself when no adapter has it.
QString nameForAddress(const QHostAddress &local);

} // namespace netif
