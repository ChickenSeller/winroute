#pragma once

#include "core/Profile.h"

#include <QDialog>
#include <QElapsedTimer>
#include <QPointer>

class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTcpSocket;
class QTextEdit;
class DnsClient;
class ProxyConnector;

// Proxy checker: tests the connection to a proxy and an HTTP request through it.
class ProxyCheckerDialog : public QDialog {
    Q_OBJECT
public:
    // Each entry is a list of hops (one proxy, or a chain). Names label the combo box.
    ProxyCheckerDialog(const QList<ProxyServer> &proxies, QWidget *parent = nullptr);
    ProxyCheckerDialog(const QList<QPair<QString, QList<ProxyServer>>> &choices, QWidget *parent = nullptr);

    void startTest();

private:
    void build();
    void out(const QString &html);
    void step1();
    void step2();
    void finish(bool ok);

    QList<QPair<QString, QList<ProxyServer>>> m_choices;
    QComboBox *m_choice = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QTextEdit *m_output = nullptr;
    QPushButton *m_start = nullptr;
    DnsClient *m_dns = nullptr;
    QPointer<QTcpSocket> m_socket;
    QPointer<ProxyConnector> m_connector;
    QList<ProxyServer> m_hops;
    QElapsedTimer m_timer;
    bool m_running = false;
};
