#include "ChainTypeDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

ChainTypeDialog::ChainTypeDialog(const ProxyChain &chain, QWidget *parent)
    : QDialog(parent), m_chain(chain)
{
    setWindowTitle(tr("Chain Type"));
    auto *layout = new QVBoxLayout(this);

    auto *simpleBox = new QGroupBox(tr("Simple Chain"));
    auto *sl = new QVBoxLayout(simpleBox);
    m_simple = new QRadioButton(tr("Connection to a remote host will be performed sequentially from one proxy server to another."));
    sl->addWidget(m_simple);
    layout->addWidget(simpleBox);

    auto *redBox = new QGroupBox(tr("Redundancy"));
    auto *rl = new QVBoxLayout(redBox);
    m_redundancy = new QRadioButton(tr("If the first proxy fails, the next one will be used, etc."));
    auto *options = new QPushButton(tr("Options..."));
    options->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rl->addWidget(m_redundancy);
    rl->addWidget(options);
    layout->addWidget(redBox);

    auto *lbBox = new QGroupBox(tr("Load Balancing"));
    auto *ll = new QVBoxLayout(lbBox);
    m_balance = new QRadioButton(tr("Each connection will be made through a random proxy from the list."));
    m_samePid = new QCheckBox(tr("Use the same proxy for the same process PID"));
    m_samePid->setChecked(chain.lbSamePidSameProxy);
    ll->addWidget(m_balance);
    ll->addWidget(m_samePid);
    layout->addWidget(lbBox);

    (chain.type == ChainType::Simple ? m_simple : chain.type == ChainType::Redundancy ? m_redundancy : m_balance)->setChecked(true);
    auto update = [this, options] {
        options->setEnabled(m_redundancy->isChecked());
        m_samePid->setEnabled(m_balance->isChecked());
    };
    for (auto *rb : {m_simple, m_redundancy, m_balance})
        connect(rb, &QRadioButton::toggled, this, update);
    update();
    connect(options, &QPushButton::clicked, this, [this] {
        RedundancyOptionsDialog dlg(m_chain, this);
        connect(&dlg, &RedundancyOptionsDialog::resetFailuresRequested, this, &ChainTypeDialog::resetFailuresRequested);
        if (dlg.exec() == QDialog::Accepted)
            dlg.apply(&m_chain);
    });

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(bb);
}

void ChainTypeDialog::apply(ProxyChain *chain) const
{
    chain->type = m_simple->isChecked() ? ChainType::Simple : m_redundancy->isChecked() ? ChainType::Redundancy : ChainType::LoadBalancing;
    chain->redundancyTimeout = m_chain.redundancyTimeout;
    chain->redundancyRecheck = m_chain.redundancyRecheck;
    chain->redundancyRecheckSec = m_chain.redundancyRecheckSec;
    chain->redundancyDirectFallback = m_chain.redundancyDirectFallback;
    chain->lbSamePidSameProxy = m_samePid->isChecked();
}

RedundancyOptionsDialog::RedundancyOptionsDialog(const ProxyChain &chain, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Redundancy Options"));
    auto *layout = new QGridLayout(this);
    m_timeout = new QSpinBox;
    m_timeout->setRange(1, 600);
    m_timeout->setValue(chain.redundancyTimeout);
    m_recheck = new QCheckBox(tr("Recheck failed proxy each (sec.):"));
    m_recheck->setChecked(chain.redundancyRecheck);
    m_recheckSec = new QSpinBox;
    m_recheckSec->setRange(1, 86400);
    m_recheckSec->setValue(chain.redundancyRecheckSec);
    m_direct = new QCheckBox(tr("Connect directly if all proxies fail"));
    m_direct->setChecked(chain.redundancyDirectFallback);
    auto *reset = new QPushButton(tr("Reset Fail Status for All Proxies"));
    layout->addWidget(new QLabel(tr("Connection to proxy timeout (sec.):")), 0, 0);
    layout->addWidget(m_timeout, 0, 1);
    layout->addWidget(m_recheck, 1, 0);
    layout->addWidget(m_recheckSec, 1, 1);
    layout->addWidget(m_direct, 2, 0, 1, 2);
    layout->addWidget(reset, 3, 0, Qt::AlignLeft);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb, 4, 0, 1, 2);
    connect(m_recheck, &QCheckBox::toggled, m_recheckSec, &QWidget::setEnabled);
    m_recheckSec->setEnabled(m_recheck->isChecked());
    connect(reset, &QPushButton::clicked, this, &RedundancyOptionsDialog::resetFailuresRequested);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RedundancyOptionsDialog::apply(ProxyChain *chain) const
{
    chain->redundancyTimeout = m_timeout->value();
    chain->redundancyRecheck = m_recheck->isChecked();
    chain->redundancyRecheckSec = m_recheckSec->value();
    chain->redundancyDirectFallback = m_direct->isChecked();
}
