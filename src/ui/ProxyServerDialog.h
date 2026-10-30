#pragma once

#include "core/Profile.h"

#include <QDialog>

#include <memory>

namespace Ui {
class ProxyServerDialog;
class ProxyAdvancedDialog;
}

// "Proxy Server" editor (address, port, protocol, authentication, options).
// The layout lives in ProxyServerDialog.ui.
class ProxyServerDialog : public QDialog {
    Q_OBJECT
public:
    ProxyServerDialog(const ProxyServer &server, bool httpSupport, QWidget *parent = nullptr);
    ~ProxyServerDialog() override;
    ProxyServer server() const;

private:
    void updateOptions();
    void accept() override;
    void onCheck();
    void onAdvanced();

    ProxyServer m_server;
    std::unique_ptr<Ui::ProxyServerDialog> ui;
};

// "Advanced Proxy Server Settings". The layout lives in ProxyAdvancedDialog.ui.
class ProxyAdvancedDialog : public QDialog {
    Q_OBJECT
public:
    ProxyAdvancedDialog(const ProxyServer &server, QWidget *parent = nullptr);
    ~ProxyAdvancedDialog() override;
    void apply(ProxyServer *server) const;

private:
    std::unique_ptr<Ui::ProxyAdvancedDialog> ui;
};
