#pragma once

#include "core/ProxyGateway.h"

#include <QDialog>

#include <memory>

namespace Ui {
class ProxyGatewayDialog;
}

// "Proxy Gateway" settings: let other machines reach the network through this
// one (HTTP/HTTPS/SOCKS5), either following this machine's proxy rules, sending
// everything through a chosen proxy or chain, or connecting directly. The
// layout lives in ProxyGatewayDialog.ui; the settings live in WinRoute.ini.
class ProxyGatewayDialog : public QDialog {
    Q_OBJECT
public:
    // The profile supplies the proxies and chains offered as a fixed exit.
    ProxyGatewayDialog(const ProxyGateway::Config &config, const Profile &profile, QWidget *parent = nullptr);
    ~ProxyGatewayDialog() override;

    ProxyGateway::Config config() const;

private:
    void update();

    std::unique_ptr<Ui::ProxyGatewayDialog> ui;
};
