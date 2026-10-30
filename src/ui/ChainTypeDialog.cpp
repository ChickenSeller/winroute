#include "ChainTypeDialog.h"
#include "ui_ChainTypeDialog.h"
#include "ui_RedundancyOptionsDialog.h"

#include <QRadioButton>

ChainTypeDialog::ChainTypeDialog(const ProxyChain &chain, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::ChainTypeDialog>()), m_chain(chain)
{
    ui->setupUi(this);

    (chain.type == ChainType::Simple        ? ui->simpleRadio
     : chain.type == ChainType::Redundancy  ? ui->redundancyRadio
                                            : ui->balanceRadio)
        ->setChecked(true);
    ui->samePidCheck->setChecked(chain.lbSamePidSameProxy);

    auto update = [this] {
        ui->optionsButton->setEnabled(ui->redundancyRadio->isChecked());
        ui->samePidCheck->setEnabled(ui->balanceRadio->isChecked());
    };
    for (auto *rb : {ui->simpleRadio, ui->redundancyRadio, ui->balanceRadio})
        connect(rb, &QRadioButton::toggled, this, update);
    update();

    connect(ui->optionsButton, &QPushButton::clicked, this, [this] {
        RedundancyOptionsDialog dlg(m_chain, this);
        connect(&dlg, &RedundancyOptionsDialog::resetFailuresRequested, this, &ChainTypeDialog::resetFailuresRequested);
        if (dlg.exec() == QDialog::Accepted)
            dlg.apply(&m_chain);
    });
}

ChainTypeDialog::~ChainTypeDialog() = default;

void ChainTypeDialog::apply(ProxyChain *chain) const
{
    chain->type = ui->simpleRadio->isChecked()       ? ChainType::Simple
                  : ui->redundancyRadio->isChecked() ? ChainType::Redundancy
                                                     : ChainType::LoadBalancing;
    chain->redundancyTimeout = m_chain.redundancyTimeout;
    chain->redundancyRecheck = m_chain.redundancyRecheck;
    chain->redundancyRecheckSec = m_chain.redundancyRecheckSec;
    chain->redundancyDirectFallback = m_chain.redundancyDirectFallback;
    chain->lbSamePidSameProxy = ui->samePidCheck->isChecked();
}

RedundancyOptionsDialog::RedundancyOptionsDialog(const ProxyChain &chain, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::RedundancyOptionsDialog>())
{
    ui->setupUi(this);

    ui->timeoutSpin->setValue(chain.redundancyTimeout);
    ui->recheckCheck->setChecked(chain.redundancyRecheck);
    ui->recheckSecSpin->setValue(chain.redundancyRecheckSec);
    ui->directCheck->setChecked(chain.redundancyDirectFallback);
    ui->recheckSecSpin->setEnabled(ui->recheckCheck->isChecked());

    connect(ui->recheckCheck, &QCheckBox::toggled, ui->recheckSecSpin, &QWidget::setEnabled);
    connect(ui->resetButton, &QPushButton::clicked, this, &RedundancyOptionsDialog::resetFailuresRequested);
}

RedundancyOptionsDialog::~RedundancyOptionsDialog() = default;

void RedundancyOptionsDialog::apply(ProxyChain *chain) const
{
    chain->redundancyTimeout = ui->timeoutSpin->value();
    chain->redundancyRecheck = ui->recheckCheck->isChecked();
    chain->redundancyRecheckSec = ui->recheckSecSpin->value();
    chain->redundancyDirectFallback = ui->directCheck->isChecked();
}
