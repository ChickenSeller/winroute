#pragma once

#include "core/Profile.h"

#include <QDialog>

class QCheckBox;
class QRadioButton;
class QSpinBox;

// "Chain Type": simple chain, redundancy or load balancing.
class ChainTypeDialog : public QDialog {
    Q_OBJECT
public:
    ChainTypeDialog(const ProxyChain &chain, QWidget *parent = nullptr);
    void apply(ProxyChain *chain) const;

signals:
    void resetFailuresRequested();

private:
    ProxyChain m_chain;
    QRadioButton *m_simple;
    QRadioButton *m_redundancy;
    QRadioButton *m_balance;
    QCheckBox *m_samePid;
};

// "Redundancy Options".
class RedundancyOptionsDialog : public QDialog {
    Q_OBJECT
public:
    RedundancyOptionsDialog(const ProxyChain &chain, QWidget *parent = nullptr);
    void apply(ProxyChain *chain) const;

signals:
    void resetFailuresRequested();

private:
    QSpinBox *m_timeout;
    QCheckBox *m_recheck;
    QSpinBox *m_recheckSec;
    QCheckBox *m_direct;
};
