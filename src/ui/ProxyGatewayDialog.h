#pragma once

#include "core/ProxyGateway.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;

// "Proxy Server" settings: let other machines reach the network through this
// one (HTTP/HTTPS/SOCKS5), either following this machine's proxy rules or
// connecting directly. The settings live in WinRoute.ini.
class ProxyGatewayDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProxyGatewayDialog(const ProxyGateway::Config &config, QWidget *parent = nullptr);

    ProxyGateway::Config config() const;

private:
    void update();

    QCheckBox *m_enabled;
    QLineEdit *m_listen;
    QCheckBox *m_httpEnabled;
    QSpinBox *m_httpPort;
    QCheckBox *m_socksEnabled;
    QSpinBox *m_socksPort;
    QRadioButton *m_modeRules;
    QRadioButton *m_modeDirect;
    QRadioButton *m_missingBlock;
    QRadioButton *m_missingDirect;
    QLineEdit *m_user;
    QLineEdit *m_pass;
};
