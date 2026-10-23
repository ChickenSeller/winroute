#pragma once

#include "core/Profile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QSpinBox;

// "Proxy Server" editor (address, port, protocol, authentication, options).
class ProxyServerDialog : public QDialog {
    Q_OBJECT
public:
    ProxyServerDialog(const ProxyServer &server, bool httpSupport, QWidget *parent = nullptr);
    ProxyServer server() const;

private:
    void updateOptions();
    void accept() override;
    void onCheck();
    void onAdvanced();

    ProxyServer m_server;
    QLineEdit *m_address;
    QSpinBox *m_port;
    QComboBox *m_protocol;
    QGroupBox *m_auth;
    QCheckBox *m_authEnable;
    QLineEdit *m_user;
    QLineEdit *m_pass;
    QCheckBox *m_socks4a;
    QLabel *m_noOptions;
};

// "Advanced Proxy Server Settings".
class ProxyAdvancedDialog : public QDialog {
    Q_OBJECT
public:
    ProxyAdvancedDialog(const ProxyServer &server, QWidget *parent = nullptr);
    void apply(ProxyServer *server) const;

private:
    QLineEdit *m_label;
    QCheckBox *m_useHostname;
};
