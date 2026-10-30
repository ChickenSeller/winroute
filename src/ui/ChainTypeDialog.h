#pragma once

#include "core/Profile.h"

#include <QDialog>

#include <memory>

namespace Ui {
class ChainTypeDialog;
class RedundancyOptionsDialog;
}

// "Chain Type": simple chain, redundancy or load balancing. Layout in
// ChainTypeDialog.ui.
class ChainTypeDialog : public QDialog {
    Q_OBJECT
public:
    ChainTypeDialog(const ProxyChain &chain, QWidget *parent = nullptr);
    ~ChainTypeDialog() override;
    void apply(ProxyChain *chain) const;

signals:
    void resetFailuresRequested();

private:
    std::unique_ptr<Ui::ChainTypeDialog> ui;
    ProxyChain m_chain;
};

// "Redundancy Options". Layout in RedundancyOptionsDialog.ui.
class RedundancyOptionsDialog : public QDialog {
    Q_OBJECT
public:
    RedundancyOptionsDialog(const ProxyChain &chain, QWidget *parent = nullptr);
    ~RedundancyOptionsDialog() override;
    void apply(ProxyChain *chain) const;

signals:
    void resetFailuresRequested();

private:
    std::unique_ptr<Ui::RedundancyOptionsDialog> ui;
};
