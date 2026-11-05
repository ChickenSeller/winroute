#pragma once

#include "core/Profile.h"

#include <QDialog>

#include <memory>

class QComboBox;

namespace Ui {
class RuleDialog;
}

// Fills an action combo box with Direct, Block, every proxy and every chain.
void fillActionCombo(QComboBox *combo, const Profile &profile, const RuleAction &current);
RuleAction comboAction(const QComboBox *combo);

// "Proxification Rule" editor. The layout lives in RuleDialog.ui.
class RuleDialog : public QDialog {
    Q_OBJECT
public:
    RuleDialog(const Rule &rule, const Profile &profile, bool isDefault, QWidget *parent = nullptr);
    ~RuleDialog() override;
    Rule rule() const;

private:
    void accept() override;
    void editInterface();        // the "Advanced" dialog: the rule's network adapter
    void updateAdvancedButton();

    Rule m_rule;
    QString m_interfaceId;       // network adapter the rule is bound to, or empty
    bool m_skipIfInterfaceDown = false;
    std::unique_ptr<Ui::RuleDialog> ui;
};
