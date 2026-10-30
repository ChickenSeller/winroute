#include "ProxyServerDialog.h"
#include "ui_ProxyServerDialog.h"
#include "ui_ProxyAdvancedDialog.h"

#include "ProxyCheckerDialog.h"

#include <QMessageBox>
#include <QPushButton>

ProxyServerDialog::ProxyServerDialog(const ProxyServer &server, bool httpSupport, QWidget *parent)
    : QDialog(parent), m_server(server), ui(std::make_unique<Ui::ProxyServerDialog>())
{
    ui->setupUi(this);

    ui->addressEdit->setText(server.address);
    ui->portSpin->setValue(server.port ? server.port : 1080);
    ui->protocolCombo->addItem(tr("SOCKS Version 4"), int(ProxyType::Socks4));
    ui->protocolCombo->addItem(tr("SOCKS Version 5"), int(ProxyType::Socks5));
    ui->protocolCombo->addItem(tr("HTTPS"), int(ProxyType::Https));
    if (httpSupport || server.type == ProxyType::Http)
        ui->protocolCombo->addItem(tr("HTTP"), int(ProxyType::Http));
    ui->protocolCombo->setCurrentIndex(qMax(0, ui->protocolCombo->findData(int(server.type))));
    ui->authEnableCheck->setChecked(server.authEnabled);
    ui->userEdit->setText(server.username);
    ui->passEdit->setText(server.password);
    ui->socks4aCheck->setChecked(server.socks4a);

    connect(ui->okButton, &QPushButton::clicked, this, &ProxyServerDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->checkButton, &QPushButton::clicked, this, &ProxyServerDialog::onCheck);
    connect(ui->advancedButton, &QPushButton::clicked, this, &ProxyServerDialog::onAdvanced);
    connect(ui->protocolCombo, &QComboBox::currentIndexChanged, this, &ProxyServerDialog::updateOptions);
    connect(ui->authEnableCheck, &QCheckBox::toggled, this, &ProxyServerDialog::updateOptions);
    updateOptions();
    setFixedWidth(sizeHint().width() + 60);
}

ProxyServerDialog::~ProxyServerDialog() = default;

void ProxyServerDialog::updateOptions()
{
    const auto type = ProxyType(ui->protocolCombo->currentData().toInt());
    const bool socks4 = type == ProxyType::Socks4;
    // SOCKS4 has only a user id, no password.
    ui->authEnableCheck->setEnabled(true);
    ui->userEdit->setEnabled(ui->authEnableCheck->isChecked());
    ui->passEdit->setEnabled(ui->authEnableCheck->isChecked() && !socks4);
    ui->socks4aCheck->setVisible(socks4);
    ui->noOptionsLabel->setVisible(!socks4);
    if (type == ProxyType::Socks5)
        ui->noOptionsLabel->setText(tr("No options for SOCKS5 are available."));
    else if (type == ProxyType::Https)
        ui->noOptionsLabel->setText(tr("The proxy must support HTTP tunneling (CONNECT requests)."));
    else
        ui->noOptionsLabel->setText(tr("HTTP proxy servers can be used for HTTP connections only."));
    if (ui->portSpin->value() == 1080 && (type == ProxyType::Https || type == ProxyType::Http) && m_server.address.isEmpty())
        ui->portSpin->setValue(8080);
    else if (ui->portSpin->value() == 8080 && (type == ProxyType::Socks4 || type == ProxyType::Socks5) && m_server.address.isEmpty())
        ui->portSpin->setValue(1080);
}

ProxyServer ProxyServerDialog::server() const
{
    ProxyServer s = m_server;
    s.address = ui->addressEdit->text().trimmed();
    s.port = quint16(ui->portSpin->value());
    s.type = ProxyType(ui->protocolCombo->currentData().toInt());
    s.authEnabled = ui->authEnableCheck->isChecked();
    s.username = ui->userEdit->text();
    s.password = ui->passEdit->text();
    s.socks4a = ui->socks4aCheck->isChecked();
    return s;
}

void ProxyServerDialog::accept()
{
    if (ui->addressEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Proxy Server"), tr("Please enter the proxy server address."));
        ui->addressEdit->setFocus();
        return;
    }
    QDialog::accept();
}

void ProxyServerDialog::onCheck()
{
    if (ui->addressEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Proxy Server"), tr("Please enter the proxy server address."));
        return;
    }
    ProxyCheckerDialog dlg({server()}, this);
    dlg.startTest();
    dlg.exec();
}

void ProxyServerDialog::onAdvanced()
{
    ProxyAdvancedDialog dlg(m_server, this);
    if (dlg.exec() == QDialog::Accepted)
        dlg.apply(&m_server);
}

ProxyAdvancedDialog::ProxyAdvancedDialog(const ProxyServer &server, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::ProxyAdvancedDialog>())
{
    ui->setupUi(this);
    ui->labelEdit->setText(server.label);
    ui->useHostnameCheck->setChecked(server.useHostname);
}

ProxyAdvancedDialog::~ProxyAdvancedDialog() = default;

void ProxyAdvancedDialog::apply(ProxyServer *server) const
{
    server->label = ui->labelEdit->text().trimmed();
    server->useHostname = ui->useHostnameCheck->isChecked();
}
