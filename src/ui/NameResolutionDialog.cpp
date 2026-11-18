#include "NameResolutionDialog.h"
#include "ui_NameResolutionDialog.h"
#include "RuleDialog.h"
#include "core/ProxyDns.h"

#include <QHostAddress>
#include <QMessageBox>
#include <QPushButton>

NameResolutionDialog::NameResolutionDialog(const Profile &profile, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::NameResolutionDialog>())
{
    ui->setupUi(this);
    const ProfileOptions &o = profile.options;

    // Which proxy resolves the names of direct connections. update() fills
    // the list: the proxies and chains while names are resolved through a
    // proxy by hand, otherwise just "This computer", which is what applies.
    m_profile = profile;
    m_directId = profile.defaultDnsId();
    connect(ui->directCombo, &QComboBox::activated, this, [this] {
        if (m_directActive)
            m_directId = ui->directCombo->currentData().toInt();
    });
    ui->directCombo->setToolTip(tr("Direct connections have no proxy of their own: "
                                   "their host names are resolved through the chosen proxy or chain."));
    ui->serverEdit->setText(o.dnsProxyServer);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        const QString server = ui->serverEdit->text().trimmed();
        QHostAddress addr;
        quint16 port;
        if (!server.isEmpty() && !ProxyDns::parseServer(server, &addr, &port)) {
            QMessageBox::warning(this, windowTitle(), tr("The DNS server must be an IP address, optionally with a port, for example 8.8.8.8 or 8.8.8.8:53."));
            ui->serverEdit->setFocus();
            return;
        }
        accept();
    });

    ui->autoCheck->setChecked(o.dnsAutoDetect);
    ui->viaProxyCheck->setChecked(o.dnsViaProxy);
    ui->localFirstCheck->setChecked(o.dnsTryLocalFirst);   // hidden: not implemented by the fake DNS responder
    (o.dnsOnlyFromList ? ui->onlyRadio : ui->excludeRadio)->setChecked(true);
    ui->listEdit->setPlainText(o.dnsExclusionList);

    connect(ui->autoCheck, &QCheckBox::toggled, this, &NameResolutionDialog::update);
    connect(ui->viaProxyCheck, &QCheckBox::toggled, this, &NameResolutionDialog::update);
    update();
}

NameResolutionDialog::~NameResolutionDialog() = default;

void NameResolutionDialog::update()
{
    // Like Proxifier: automatic mode decides by itself, so the manual switch is disabled.
    ui->viaProxyCheck->setEnabled(!ui->autoCheck->isChecked());
    const bool lists = ui->viaProxyCheck->isChecked() || ui->autoCheck->isChecked();
    ui->excludeRadio->setEnabled(lists);
    ui->onlyRadio->setEnabled(lists);
    ui->listEdit->setEnabled(lists);
    // The resolver choices apply only when names are set to be resolved
    // through a proxy by hand; automatic detection uses the defaults.
    const bool active = ui->viaProxyCheck->isChecked() && !ui->autoCheck->isChecked();
    ui->resolverGroup->setEnabled(active);
    if (active != m_directActive || ui->directCombo->count() == 0) {
        m_directActive = active;
        if (active) {
            // Resolving through a proxy is the point, so this computer is no
            // choice; without any proxy, direct connections still resolve locally.
            fillResolverCombo(ui->directCombo, m_profile, QString(), m_directId);
            if (ui->directCombo->count() == 0)
                ui->directCombo->addItem(tr("(no proxy servers)"), 0);
            m_directId = ui->directCombo->currentData().toInt();
        } else {
            ui->directCombo->clear();
            ui->directCombo->addItem(tr("This computer"), 0);
        }
    }
    ui->directCombo->setEnabled(active && ui->directCombo->currentData().toInt() != 0);
}

void NameResolutionDialog::apply(ProfileOptions *o) const
{
    o->dnsAutoDetect = ui->autoCheck->isChecked();
    o->dnsViaProxy = ui->viaProxyCheck->isChecked();
    o->dnsTryLocalFirst = ui->localFirstCheck->isChecked();
    o->dnsOnlyFromList = ui->onlyRadio->isChecked();
    o->dnsExclusionList = ui->listEdit->toPlainText().simplified();
    // Kept as it was while it does not apply.
    if (m_directActive)
        o->dnsDirectProxyId = m_directId;
    const QString server = ui->serverEdit->text().trimmed();
    o->dnsProxyServer = server.isEmpty() ? ProfileOptions().dnsProxyServer : server;
}
