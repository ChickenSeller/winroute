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
// Fills a combo box with `first` (data 0; left out when empty), then every
// proxy and chain (data: its id), and selects `current` (else the first item).
// For choosing the proxy that resolves names.
void fillResolverCombo(QComboBox *combo, const Profile &profile, const QString &first, int current);

// "Proxification Rule" editor. The layout lives in RuleDialog.ui.
class RuleDialog : public QDialog {
    Q_OBJECT
public:
    RuleDialog(const Rule &rule, const Profile &profile, bool isDefault, QWidget *parent = nullptr);
    ~RuleDialog() override;
    Rule rule() const;

private:
    void accept() override;
    void editInterface();        // the "Advanced" dialog: network adapter and name resolution
    void updateAdvancedButton();

    Rule m_rule;
    QString m_interfaceId;       // network adapter the rule is bound to, or empty
    bool m_skipIfInterfaceDown = false;
    DnsResolve m_resolve = DnsResolve::Inherit;   // the rule's name resolution
    int m_resolveProxyId = 0;                     // and the proxy that does it (0: the exit)
    Profile m_profile;                            // for the proxy and chain names
    std::unique_ptr<Ui::RuleDialog> ui;
};
