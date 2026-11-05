#include "RuleDialog.h"
#include "ui_RuleDialog.h"

#include "Icons.h"
#include "core/NetInterfaces.h"
#include "core/Utils.h"

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QRegularExpression>

namespace {

// A combo box whose long entries (adapter names with their description and
// addresses) do not widen it or its drop-down: the text is elided to the box's
// width and the full text is the tooltip, of each entry and of the box.
class ElidedComboBox : public QComboBox {
public:
    explicit ElidedComboBox(QWidget *parent = nullptr) : QComboBox(parent)
    {
        setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        setMinimumContentsLength(20);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        view()->setTextElideMode(Qt::ElideRight);
        view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        connect(this, &QComboBox::currentIndexChanged, this, [this](int i) { setToolTip(itemText(i)); });
    }

    // Adds an entry whose full text is also its tooltip.
    void addEntry(const QString &text, const QVariant &data)
    {
        addItem(text, data);
        setItemData(count() - 1, text, Qt::ToolTipRole);
        if (count() == 1)
            setToolTip(text);
    }

protected:
    void showPopup() override
    {
        // Keep the drop-down as wide as the box instead of the longest entry
        // (cleared first, so it follows the box when the dialog is resized).
        QWidget *popup = view()->window();
        if (popup && popup != window()) {
            popup->setMinimumWidth(0);
            popup->setMaximumWidth(QWIDGETSIZE_MAX);
        }
        QComboBox::showPopup();
        if (popup && popup != window() && popup->width() > width())
            popup->setFixedWidth(width());
    }
};

} // namespace

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

    // "Advanced": the network adapter the rule's connections leave through.
    m_interfaceId = rule.action.interfaceId;
    m_skipIfInterfaceDown = rule.action.skipIfInterfaceDown;
    ui->advancedButton->setEnabled(true);
    connect(ui->advancedButton, &QPushButton::clicked, this, &RuleDialog::editInterface);
    connect(ui->actionCombo, &QComboBox::currentIndexChanged, this, &RuleDialog::updateAdvancedButton);
    updateAdvancedButton();

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

void RuleDialog::updateAdvancedButton()
{
    // Blocked connections are not made, so they have no adapter.
    const bool block = comboAction(ui->actionCombo).type == ActionType::Block;
    ui->advancedButton->setEnabled(!block);
    if (m_interfaceId.isEmpty() || block) {
        ui->advancedButton->setText(tr("Advanced..."));
        ui->advancedButton->setToolTip(tr("Network interface: System Default"));
    } else {
        const QString name = netif::displayName(m_interfaceId);
        ui->advancedButton->setText(tr("Interface: %1...").arg(name));
        ui->advancedButton->setToolTip(tr("Network interface: %1").arg(name));
    }
}

void RuleDialog::editInterface()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Rule Network Interface"));
    auto *layout = new QVBoxLayout(&dlg);
    auto *intro = new QLabel(tr("Connections matching this rule leave through this network interface: "
                                "direct connections to their target, proxied ones to the first proxy."),
                             &dlg);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *combo = new ElidedComboBox(&dlg);
    combo->addEntry(tr("System Default (chosen by the routing table)"), QString());
    bool found = m_interfaceId.isEmpty();
    for (const netif::Adapter &a : netif::adapters()) {
        if (a.loopback)
            continue;
        QStringList ips;
        for (const QHostAddress &ip : a.addresses)
            if (!ip.isLinkLocal())
                ips << ip.toString();
        QString text = a.name;
        if (!a.description.isEmpty() && a.description != a.name)
            text += QStringLiteral(" - ") + a.description;
        text += a.up ? (ips.isEmpty() ? QString() : QStringLiteral("  (") + ips.join(QStringLiteral(", ")) + u')')
                     : QStringLiteral("  ") + tr("(disconnected)");
        combo->addEntry(text, a.id);
        if (a.id.compare(m_interfaceId, Qt::CaseInsensitive) == 0) {
            combo->setCurrentIndex(combo->count() - 1);
            found = true;
        }
    }
    if (!found) {   // chosen on another machine, or the adapter was removed
        combo->addEntry(tr("%1 (not present)").arg(m_interfaceId), m_interfaceId);
        combo->setCurrentIndex(combo->count() - 1);
    }
    auto *form = new QFormLayout;
    form->addRow(tr("Interface:"), combo);
    layout->addLayout(form);

    auto *downLabel = new QLabel(tr("When the interface is down (disconnected):"), &dlg);
    auto *useDefault = new QRadioButton(tr("Use the system default interface"), &dlg);
    auto *ignore = new QRadioButton(tr("Ignore this rule (the next matching rule applies)"), &dlg);
    auto *group = new QButtonGroup(&dlg);
    group->addButton(useDefault);
    group->addButton(ignore);
    (m_skipIfInterfaceDown ? ignore : useDefault)->setChecked(true);
    layout->addWidget(downLabel);
    layout->addWidget(useDefault);
    layout->addWidget(ignore);

    const auto sync = [=] {
        const bool any = !combo->currentData().toString().isEmpty();
        downLabel->setEnabled(any);
        useDefault->setEnabled(any);
        ignore->setEnabled(any);
    };
    connect(combo, &QComboBox::currentIndexChanged, &dlg, sync);
    sync();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);
    // A moderate starting width; the dialog can be resized to show more.
    dlg.resize(440, dlg.sizeHint().height());
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_interfaceId = combo->currentData().toString();
    m_skipIfInterfaceDown = !m_interfaceId.isEmpty() && ignore->isChecked();
    updateAdvancedButton();
}

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
    if (r.action.type != ActionType::Block) {
        r.action.interfaceId = m_interfaceId;
        r.action.skipIfInterfaceDown = !m_interfaceId.isEmpty() && m_skipIfInterfaceDown;
    }
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
