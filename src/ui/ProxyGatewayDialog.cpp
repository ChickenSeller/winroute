#include "ProxyGatewayDialog.h"
#include "ui_ProxyGatewayDialog.h"

#include <QRadioButton>

ProxyGatewayDialog::ProxyGatewayDialog(const ProxyGateway::Config &c, const Profile &profile, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::ProxyGatewayDialog>())
{
    constexpr int kTypeRole = Qt::UserRole + 1;   // the exit combo stores id + type
    ui->setupUi(this);

    ui->enabledCheck->setChecked(c.enabled);
    ui->listenEdit->setText(c.listenAddress.isNull() ? QString() : c.listenAddress.toString());
    ui->httpCheck->setChecked(c.httpEnabled);
    ui->httpPort->setValue(c.httpPort ? c.httpPort : 8080);
    ui->socksCheck->setChecked(c.socksEnabled);
    ui->socksPort->setValue(c.socksPort ? c.socksPort : 1080);

    // The fixed exit: one proxy or chain of the current profile.
    for (const ProxyServer &p : profile.proxies) {
        ui->exitCombo->addItem(profile.actionDisplayName({ActionType::Proxy, p.id}), p.id);
        ui->exitCombo->setItemData(ui->exitCombo->count() - 1, int(ActionType::Proxy), kTypeRole);
    }
    for (const ProxyChain &ch : profile.chains) {
        ui->exitCombo->addItem(profile.actionDisplayName({ActionType::Chain, ch.id}), ch.id);
        ui->exitCombo->setItemData(ui->exitCombo->count() - 1, int(ActionType::Chain), kTypeRole);
    }
    for (int i = 0; i < ui->exitCombo->count(); ++i) {   // preselect the saved exit
        if (ui->exitCombo->itemData(i).toInt() == c.exit.id
            && ui->exitCombo->itemData(i, kTypeRole).toInt() == int(c.exit.type)) {
            ui->exitCombo->setCurrentIndex(i);
            break;
        }
    }
    ui->proxyRadio->setEnabled(ui->exitCombo->count() > 0);   // nothing to send through

    QRadioButton *initial = ui->rulesRadio;
    if (c.mode == ProxyGateway::Mode::Direct)
        initial = ui->directRadio;
    else if (c.mode == ProxyGateway::Mode::Proxy && ui->proxyRadio->isEnabled())
        initial = ui->proxyRadio;
    initial->setChecked(true);

    (c.onMissing == ProxyGateway::OnMissing::Direct ? ui->connectDirectRadio : ui->blockRadio)->setChecked(true);

    ui->userEdit->setText(c.username);
    ui->passEdit->setText(c.password);

    connect(ui->rulesRadio, &QRadioButton::toggled, this, &ProxyGatewayDialog::update);
    connect(ui->proxyRadio, &QRadioButton::toggled, this, &ProxyGatewayDialog::update);
    connect(ui->httpCheck, &QCheckBox::toggled, this, &ProxyGatewayDialog::update);
    connect(ui->socksCheck, &QCheckBox::toggled, this, &ProxyGatewayDialog::update);
    update();
}

ProxyGatewayDialog::~ProxyGatewayDialog() = default;

void ProxyGatewayDialog::update()
{
    ui->httpPort->setEnabled(ui->httpCheck->isChecked());
    ui->socksPort->setEnabled(ui->socksCheck->isChecked());
    ui->exitCombo->setEnabled(ui->proxyRadio->isChecked());
    // The missing-proxy fallback matters whenever a proxy might be used.
    const bool usesProxy = ui->rulesRadio->isChecked() || ui->proxyRadio->isChecked();
    ui->blockRadio->setEnabled(usesProxy);
    ui->connectDirectRadio->setEnabled(usesProxy);
}

ProxyGateway::Config ProxyGatewayDialog::config() const
{
    ProxyGateway::Config c;
    c.enabled = ui->enabledCheck->isChecked();
    const QString addr = ui->listenEdit->text().trimmed();
    c.listenAddress = addr.isEmpty() ? QHostAddress(QHostAddress::Any) : QHostAddress(addr);
    if (c.listenAddress.isNull())            // unparseable text: fall back to all interfaces
        c.listenAddress = QHostAddress(QHostAddress::Any);
    c.httpEnabled = ui->httpCheck->isChecked();
    c.httpPort = quint16(ui->httpPort->value());
    c.socksEnabled = ui->socksCheck->isChecked();
    c.socksPort = quint16(ui->socksPort->value());
    c.mode = ui->directRadio->isChecked()  ? ProxyGateway::Mode::Direct
             : ui->proxyRadio->isChecked() ? ProxyGateway::Mode::Proxy
                                           : ProxyGateway::Mode::Rules;
    if (ui->exitCombo->currentIndex() >= 0) {
        c.exit.id = ui->exitCombo->currentData().toInt();
        c.exit.type = ActionType(ui->exitCombo->currentData(Qt::UserRole + 1).toInt());
    }
    c.onMissing = ui->connectDirectRadio->isChecked() ? ProxyGateway::OnMissing::Direct : ProxyGateway::OnMissing::Block;
    c.username = ui->userEdit->text();
    c.password = ui->passEdit->text();
    return c;
}
