#include "NameResolutionDialog.h"
#include "ui_NameResolutionDialog.h"

NameResolutionDialog::NameResolutionDialog(const ProfileOptions &o, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::NameResolutionDialog>())
{
    ui->setupUi(this);

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
}

void NameResolutionDialog::apply(ProfileOptions *o) const
{
    o->dnsAutoDetect = ui->autoCheck->isChecked();
    o->dnsViaProxy = ui->viaProxyCheck->isChecked();
    o->dnsTryLocalFirst = ui->localFirstCheck->isChecked();
    o->dnsOnlyFromList = ui->onlyRadio->isChecked();
    o->dnsExclusionList = ui->listEdit->toPlainText().simplified();
}
