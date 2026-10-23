#pragma once

#include "core/Profile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;

// Fills an action combo box with Direct, Block, every proxy and every chain.
void fillActionCombo(QComboBox *combo, const Profile &profile, const RuleAction &current);
RuleAction comboAction(const QComboBox *combo);

// "Proxification Rule" editor.
class RuleDialog : public QDialog {
    Q_OBJECT
public:
    RuleDialog(const Rule &rule, const Profile &profile, bool isDefault, QWidget *parent = nullptr);
    Rule rule() const;

private:
    void accept() override;

    Rule m_rule;
    QLineEdit *m_name;
    QCheckBox *m_enabled;
    QPlainTextEdit *m_apps;
    QPlainTextEdit *m_targets;
    QLineEdit *m_ports;
    QComboBox *m_action;
};
