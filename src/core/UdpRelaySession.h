#pragma once

#include "Profile.h"
#include "Relay.h"

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <functional>

class QTcpSocket;
class QUdpSocket;
struct NetFilterApi;

// Tunnels one application UDP socket (one driver endpoint id) through a SOCKS5
// proxy using UDP ASSOCIATE (RFC 1928). Lives in the relay thread.
//
// A TCP control connection to the proxy performs the SOCKS5 greeting/auth and
// the ASSOCIATE command; the proxy replies with the address of its UDP relay.
// Outgoing datagrams are wrapped in a SOCKS5 UDP request header and sent to that
// relay; replies are unwrapped and injected back into the application socket
// with nf_udpPostReceive, as if they came from the address the application used.
//
// If the proxy cannot associate (e.g. it does not support UDP), the session
// falls back to sending the datagrams out directly with nf_udpPostSend, so the
// flow is not silently dropped.
class UdpRelaySession : public QObject {
    Q_OBJECT
public:
    UdpRelaySession(Relay *relay, const UdpDatagram &first, const ProxyServer &proxy, QObject *parent = nullptr);

    quint64 id() const { return m_id; }
    qint64 sent() const { return m_sent; }
    qint64 received() const { return m_received; }
    bool trafficChanged();

    void send(const UdpDatagram &dg);
    void close();

    // "host:port" (or "ip:port") for a datagram's target, for the connection list.
    static QString targetText(const QByteArray &remote, const QString &hostname);

    // Deliver replies and bypassed datagrams to callbacks instead of the NetFilter
    // driver API. Used by the WinDivert backend (which injects the packets itself)
    // and by tests (no driver needed). Must be set before the event loop runs the
    // deferred start().
    using DeliverSink = std::function<void(const QByteArray &appRemote, const QByteArray &payload)>;
    using BypassSink = std::function<void(const UdpDatagram &dg)>;
    void setSinks(DeliverSink deliver, BypassSink bypass)
    {
        m_deliverSink = std::move(deliver);
        m_bypassSink = std::move(bypass);
    }

private:
    enum class Stage { Resolving, Connecting, Greeting, Auth, Associate, Ready, Bypass, Closed };

    void start();
    void openControl(const QHostAddress &proxyIp);
    void onControlReadyRead();
    void onControlClosed();
    void sendGreeting();
    void sendAuth();
    void sendAssociate();
    bool parseAssociateReply();
    void openUdp(const QHostAddress &relayAddr, quint16 relayPort);
    void onUdpReadyRead();
    void flushPending();
    void sendThrough(const UdpDatagram &dg);     // wrap + send to the relay
    void bypass(const UdpDatagram &dg);          // nf_udpPostSend, direct
    void toBypass(const QString &reason);        // switch the whole session to bypass
    void deliver(const QHostAddress &origin, quint16 oport, const QByteArray &payload);
    void touch();                                 // reset the idle timer
    void finish();

    Relay *m_relay;
    const NetFilterApi *m_api;
    quint64 m_id;
    ProxyServer m_proxy;
    QString m_appName;
    QString m_targetText;

    QPointer<QTcpSocket> m_control;
    QPointer<QUdpSocket> m_udp;
    QByteArray m_ctlBuf;
    QHostAddress m_relayAddr;
    quint16 m_relayPort = 0;

    Stage m_stage = Stage::Resolving;
    QList<UdpDatagram> m_pending;      // datagrams awaiting the association
    QByteArray m_lastRemote;           // sockaddr of the app's last target
    QByteArray m_lastOpts;             // NF_UDP_OPTIONS for the reply injection
    QHash<QString, QByteArray> m_remoteByOrigin;   // real origin "ip:port" -> app sockaddr

    QTimer m_idle;
    qint64 m_sent = 0, m_received = 0;
    qint64 m_lastSent = 0, m_lastReceived = 0;
    bool m_finished = false;
    bool m_warnedBypass = false;

    DeliverSink m_deliverSink;
    BypassSink m_bypassSink;
};
