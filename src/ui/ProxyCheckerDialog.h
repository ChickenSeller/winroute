#pragma once

#include "core/Profile.h"

#include <QDialog>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QPointer>

#include <memory>

namespace Ui {
class ProxyCheckerDialog;
}

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTcpSocket;
class QTextEdit;
class QUdpSocket;
class DnsClient;
class ProxyConnector;

// Proxy checker: tests the connection to a proxy and an HTTP request through it.
class ProxyCheckerDialog : public QDialog {
    Q_OBJECT
public:
    // Each entry is a list of hops (one proxy, or a chain). Names label the combo box.
    ProxyCheckerDialog(const QList<ProxyServer> &proxies, QWidget *parent = nullptr);
    ProxyCheckerDialog(const QList<QPair<QString, QList<ProxyServer>>> &choices, QWidget *parent = nullptr);
    ~ProxyCheckerDialog() override;

    void startTest();

private:
    void build();
    void out(const QString &html);
    void step1();
    void step2();
    void httpRequest(const QString &host, const QByteArray &leftover);
    void afterTcpTests();
    void finish(bool ok);

    // UDP support test (SOCKS5 UDP ASSOCIATE), for a single SOCKS5 proxy.
    void udpTest();
    void udpCtrlReadyRead();
    void sendUdpAssociate();
    void udpProbe();
    void onUdpDatagram();
    void udpDone(bool ok, const QString &detail = {});
    void cleanupUdp();

    // Enhanced UDP probe. The DNS test above only proves the relay can carry a
    // single small round-trip to a universally reachable server (8.8.8.8:53).
    // Real game traffic needs UDP to arbitrary hosts on arbitrary ports, often
    // many flows at once, and a cone NAT at the exit. STUN exercises exactly
    // that: two public STUN servers on non-53 ports, probed concurrently; a
    // reply proves arbitrary-host reachability, and comparing the public
    // address each server reports classifies the exit's NAT mapping
    // (endpoint-independent / full-cone vs address/port-dependent / symmetric).
    void saveStunSettings();
    void udpDnsOk();
    void stunTest();
    void startStunProbes();
    void sendStun(int idx);
    void handleStunDatagram(const QByteArray &payload);
    void stunFinish();

    struct StunTarget {
        QString name;
        quint16 port = 0;
        QHostAddress ip;
        QByteArray txid;        // 12-byte transaction id
        bool replied = false;
        QHostAddress mappedAddr;
        quint16 mappedPort = 0;
    };

    std::unique_ptr<Ui::ProxyCheckerDialog> ui;
    QList<QPair<QString, QList<ProxyServer>>> m_choices;
    QComboBox *m_choice = nullptr;
    QLineEdit *m_host = nullptr;
    QLineEdit *m_stunEdit = nullptr;
    QSpinBox *m_port = nullptr;
    QTextEdit *m_output = nullptr;
    QPushButton *m_start = nullptr;
    DnsClient *m_dns = nullptr;
    QPointer<QTcpSocket> m_socket;
    QPointer<ProxyConnector> m_connector;
    QList<ProxyServer> m_hops;
    QElapsedTimer m_timer;
    bool m_running = false;
    // Counts test runs. Each timer belongs to the run that started it and
    // does nothing once another run has begun.
    int m_run = 0;

    // HTTP request test state.
    QByteArray m_httpBuf;
    bool m_httpDone = false;

    // UDP test state.
    QPointer<QTcpSocket> m_udpCtrl;
    QPointer<QUdpSocket> m_udp;
    QByteArray m_udpBuf;
    int m_udpStage = 0;          // 0 greeting, 1 auth, 2 associate
    bool m_udpPending = false;
    QHostAddress m_relayAddr;
    quint16 m_relayPort = 0;
    quint16 m_dnsId = 0;
    int m_udpPhase = 0;          // 0 DNS probe, 1 STUN probe

    // STUN probe state.
    QList<QPair<QCheckBox *, QString>> m_stunPresets;   // checkbox -> "host:port"
    QList<StunTarget> m_stun;
    int m_stunReplies = 0;
    int m_stunResolving = 0;
};
