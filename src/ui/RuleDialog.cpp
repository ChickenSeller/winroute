#include "RuleDialog.h"
#include "ui_RuleDialog.h"

#include "Icons.h"
#include "core/Utils.h"

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>

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

RuleDialog::RuleDialog(const Rule &rule, const Profile &profile, bool isDefault, QWidget *parent)
    : QDialog(parent), m_rule(rule), ui(std::make_unique<Ui::RuleDialog>())
{
    ui->setupUi(this);

    ui->nameEdit->setText(rule.name);
    ui->enabledCheck->setChecked(rule.enabled || isDefault);
    ui->enabledCheck->setEnabled(!isDefault);
    ui->appsEdit->setPlainText(rule.applications);
    ui->targetsEdit->setPlainText(rule.targets);
    ui->portsEdit->setText(rule.ports);
    fillActionCombo(ui->actionCombo, profile, rule.action);

    // The Browse button drops a menu to add files or a folder.
    auto *browseMenu = new QMenu(ui->browseButton);
    QAction *addFiles = browseMenu->addAction(tr("Add Files..."));
    QAction *addFolder = browseMenu->addAction(tr("Add Folder..."));
    ui->browseButton->setMenu(browseMenu);

    if (isDefault) {
        // The Default rule matches everything; only its action is editable.
        for (QWidget *w : {static_cast<QWidget *>(ui->appsGroup), static_cast<QWidget *>(ui->targetsGroup), static_cast<QWidget *>(ui->portsGroup)})
            w->setEnabled(false);
        ui->nameEdit->setEnabled(false);
    }

    auto appendEntry = [this](const QString &entry) {
        QString text = ui->appsEdit->toPlainText().trimmed();
        if (!text.isEmpty())
            text += "; ";
        text += entry.contains(' ') ? '"' + entry + '"' : entry;
        ui->appsEdit->setPlainText(text);
    };
    connect(addFiles, &QAction::triggered, this, [this, appendEntry] {
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Applications"), QString(),
                                                                tr("Applications (*.exe);;All files (*.*)"));
        for (const QString &f : files)
            appendEntry(QFileInfo(f).fileName());
    });
    // A folder, stored with a trailing backslash, matches every program under
    // it, including in subfolders (RuleEngine::matchAppList).
    connect(addFolder, &QAction::triggered, this, [this, appendEntry] {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select a Folder"));
        if (dir.isEmpty())
            return;
        dir.replace(u'/', u'\\');
        if (!dir.endsWith(u'\\'))
            dir += u'\\';
        appendEntry(dir);
    });
}

RuleDialog::~RuleDialog() = default;

Rule RuleDialog::rule() const
{
    Rule r = m_rule;
    r.name = ui->nameEdit->text().trimmed();
    r.enabled = ui->enabledCheck->isChecked();
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
    r.applications = util::joinAppList(ui->appsEdit->toPlainText());
    r.targets = oneLine(ui->targetsEdit->toPlainText());
    r.ports = ui->portsEdit->text().trimmed();
    r.action = comboAction(ui->actionCombo);
    return r;
}

void RuleDialog::accept()
{
    if (ui->nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Proxification Rule"), tr("Please enter the rule name."));
        ui->nameEdit->setFocus();
        return;
    }
    static const QRegularExpression portsRe(R"(^\s*(\d{1,5}(\s*-\s*\d{1,5})?)?(\s*[;,]\s*\d{1,5}(\s*-\s*\d{1,5})?)*\s*;?\s*$)");
    if (!portsRe.match(ui->portsEdit->text()).hasMatch()) {
        QMessageBox::warning(this, tr("Proxification Rule"), tr("Target ports must look like: 80; 8000-9000; 3128"));
        ui->portsEdit->setFocus();
        return;
    }
    QDialog::accept();
}
