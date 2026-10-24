#include "RuleDialog.h"
#include "Icons.h"
#include "core/Utils.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

void fillActionCombo(QComboBox *combo, const Profile &profile, const RuleAction &current)
{
    combo->clear();
    auto add = [&](const QIcon &icon, const QString &text, ActionType t, int id) {
        combo->addItem(icon, text, QVariantList{int(t), id});
        if (current.type == t && (t == ActionType::Direct || t == ActionType::Block || current.id == id))
            combo->setCurrentIndex(combo->count() - 1);
    };
    add(icons::direct(), QCoreApplication::translate("RuleDialog", "Direct"), ActionType::Direct, 0);
    add(icons::block(), QCoreApplication::translate("RuleDialog", "Block"), ActionType::Block, 0);
    for (const ProxyServer &p : profile.proxies)
        add(icons::proxy(), QCoreApplication::translate("RuleDialog", "Proxy %1 %2").arg(proxyTypeToString(p.type), p.displayName()), ActionType::Proxy, p.id);
    for (const ProxyChain &c : profile.chains)
        add(icons::chain(), QCoreApplication::translate("RuleDialog", "Chain %1").arg(c.name), ActionType::Chain, c.id);
}

RuleAction comboAction(const QComboBox *combo)
{
    const QVariantList v = combo->currentData().toList();
    if (v.size() != 2)
        return {};
    return {ActionType(v[0].toInt()), v[1].toInt()};
}

static QLabel *exampleLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setEnabled(false);
    return l;
}

RuleDialog::RuleDialog(const Rule &rule, const Profile &profile, bool isDefault, QWidget *parent)
    : QDialog(parent), m_rule(rule)
{
    setWindowTitle(tr("Proxification Rule"));
    resize(560, 520);
    auto *layout = new QVBoxLayout(this);

    auto *nameRow = new QHBoxLayout;
    m_name = new QLineEdit(rule.name);
    m_enabled = new QCheckBox(tr("Enabled"));
    m_enabled->setChecked(rule.enabled || isDefault);
    m_enabled->setEnabled(!isDefault);
    nameRow->addWidget(new QLabel(tr("Name:")));
    nameRow->addWidget(m_name, 1);
    nameRow->addWidget(m_enabled);
    layout->addLayout(nameRow);

    auto *appBox = new QGroupBox(tr("Applications"));
    auto *al = new QGridLayout(appBox);
    m_apps = new QPlainTextEdit(rule.applications);
    m_apps->setPlaceholderText(tr("Any"));
    m_apps->setMaximumHeight(70);
    auto *browse = new QPushButton(tr("Browse..."));
    al->addWidget(m_apps, 0, 0, 1, 2);
    al->addWidget(exampleLabel(tr("Example: iexplore.exe; \"C:\\some app.exe\"; fire*.exe; *.bin")), 1, 0);
    al->addWidget(browse, 1, 1);
    layout->addWidget(appBox);

    auto *targetBox = new QGroupBox(tr("Target hosts"));
    auto *tl = new QVBoxLayout(targetBox);
    m_targets = new QPlainTextEdit(rule.targets);
    m_targets->setPlaceholderText(tr("Any"));
    m_targets->setMaximumHeight(70);
    tl->addWidget(m_targets);
    tl->addWidget(exampleLabel(tr("Example: 127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255")));
    layout->addWidget(targetBox);

    auto *portBox = new QGroupBox(tr("Target ports"));
    auto *pl = new QVBoxLayout(portBox);
    m_ports = new QLineEdit(rule.ports);
    m_ports->setPlaceholderText(tr("Any"));
    pl->addWidget(m_ports);
    pl->addWidget(exampleLabel(tr("Example: 80; 8000-9000; 3128")));
    layout->addWidget(portBox);

    auto *actionRow = new QHBoxLayout;
    m_action = new QComboBox;
    fillActionCombo(m_action, profile, rule.action);
    auto *advanced = new QPushButton(tr("Advanced..."));
    advanced->setEnabled(false);
    advanced->setToolTip(tr("Per-rule network interface selection is not supported yet"));
    actionRow->addWidget(new QLabel(tr("Action:")));
    actionRow->addWidget(m_action, 1);
    actionRow->addWidget(advanced);
    layout->addLayout(actionRow);

    if (isDefault) {
        // The Default rule matches everything; only its action is editable.
        for (QWidget *w : {static_cast<QWidget *>(appBox), static_cast<QWidget *>(targetBox), static_cast<QWidget *>(portBox)})
            w->setEnabled(false);
        m_name->setEnabled(false);
    }

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &RuleDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, [this] {
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Applications"), QString(),
                                                                tr("Applications (*.exe);;All files (*.*)"));
        if (files.isEmpty())
            return;
        QString text = m_apps->toPlainText().trimmed();
        for (const QString &f : files) {
            const QString name = QFileInfo(f).fileName();
            if (!text.isEmpty())
                text += "; ";
            text += name.contains(' ') ? '"' + name + '"' : name;
        }
        m_apps->setPlainText(text);
    });
}

Rule RuleDialog::rule() const
{
    Rule r = m_rule;
    r.name = m_name->text().trimmed();
    r.enabled = m_enabled->isChecked();
    auto oneLine = [](const QString &s) {
        QStringList parts;
        for (const QString &line : s.split('\n')) {
            const QString t = line.trimmed();
            if (!t.isEmpty())
                parts << (t.endsWith(';') ? t.chopped(1).trimmed() : t);
        }
        return parts.join("; ");
    };
    // Quote application names containing spaces (Proxifier's required form).
    r.applications = util::joinAppList(m_apps->toPlainText());
    r.targets = oneLine(m_targets->toPlainText());
    r.ports = m_ports->text().trimmed();
    r.action = comboAction(m_action);
    return r;
}

void RuleDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Proxification Rule"), tr("Please enter the rule name."));
        m_name->setFocus();
        return;
    }
    static const QRegularExpression portsRe(R"(^\s*(\d{1,5}(\s*-\s*\d{1,5})?)?(\s*[;,]\s*\d{1,5}(\s*-\s*\d{1,5})?)*\s*;?\s*$)");
    if (!portsRe.match(m_ports->text()).hasMatch()) {
        QMessageBox::warning(this, tr("Proxification Rule"), tr("Target ports must look like: 80; 8000-9000; 3128"));
        m_ports->setFocus();
        return;
    }
    QDialog::accept();
}
