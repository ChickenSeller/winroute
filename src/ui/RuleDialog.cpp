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
#include <QGroupBox>
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

void fillResolverCombo(QComboBox *combo, const Profile &profile, const QString &first, int current)
{
    combo->clear();
    if (!first.isEmpty())
        combo->addItem(first, 0);
    for (const ProxyServer &p : profile.proxies)
        combo->addItem(icons::proxy(), QCoreApplication::translate("RuleDialog", "Proxy %1 %2").arg(proxyTypeToString(p.type), p.displayName()), p.id);
    for (const ProxyChain &c : profile.chains)
        combo->addItem(icons::chain(), QCoreApplication::translate("RuleDialog", "Chain %1").arg(c.name), c.id);
    const int i = combo->findData(current);
    combo->setCurrentIndex(i < 0 ? 0 : i);
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

    // "Advanced": the network adapter the rule's connections leave through,
    // and how their host names are resolved.
    m_interfaceId = rule.action.interfaceId;
    m_skipIfInterfaceDown = rule.action.skipIfInterfaceDown;
    ui->advancedButton->setEnabled(true);
    connect(ui->advancedButton, &QPushButton::clicked, this, &RuleDialog::editInterface);
    connect(ui->actionCombo, &QComboBox::currentIndexChanged, this, &RuleDialog::updateAdvancedButton);
    m_resolve = rule.resolve;
    m_resolveProxyId = rule.resolveProxyId;
    m_profile = profile;
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
    if (m_resolve != DnsResolve::Inherit && !block)
        ui->advancedButton->setToolTip(ui->advancedButton->toolTip() + u'\n'
                                       + (m_resolve == DnsResolve::Local ? tr("Host names: resolved on this computer")
                                          : m_resolveProxyId ? tr("Host names: resolved through %1").arg(m_profile.actionDisplayName(
                                                                   {m_profile.proxyById(m_resolveProxyId) ? ActionType::Proxy : ActionType::Chain, m_resolveProxyId}))
                                                             : tr("Host names: resolved through the rule's exit")));
}

void RuleDialog::editInterface()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Advanced Rule Settings"));
    auto *outer = new QVBoxLayout(&dlg);
    auto *ifaceBox = new QGroupBox(tr("Network Interface"), &dlg);
    auto *layout = new QVBoxLayout(ifaceBox);
    outer->addWidget(ifaceBox);
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

    // Name resolution of this rule's connections, over the profile's setting.
    // Only a proxy resolves names; direct connections always resolve locally.
    auto *dnsBox = new QGroupBox(tr("Name Resolution"), &dlg);
    auto *dnsLayout = new QVBoxLayout(dnsBox);
    auto *dnsIntro = new QLabel(tr("How host names of this rule's connections are resolved. "
                                   "This overrides the profile's Name Resolution setting for this rule."),
                                &dlg);
    dnsIntro->setWordWrap(true);
    dnsLayout->addWidget(dnsIntro);
    auto *resolveCombo = new QComboBox(&dlg);
    resolveCombo->addItem(tr("As set in Name Resolution"), int(DnsResolve::Inherit));
    resolveCombo->addItem(tr("Through a proxy"), int(DnsResolve::Proxy));
    resolveCombo->addItem(tr("On this computer"), int(DnsResolve::Local));
    resolveCombo->setCurrentIndex(qMax(0, resolveCombo->findData(int(m_resolve))));
    const ActionType type = comboAction(ui->actionCombo).type;
    const bool direct = type == ActionType::Direct;
    auto *resolverCombo = new ElidedComboBox(&dlg);
    fillResolverCombo(resolverCombo, m_profile,
                      direct ? tr("The rule's exit (Direct: the resolver for direct connections in Name Resolution)") : tr("The rule's exit"),
                      m_resolveProxyId);
    auto *dnsForm = new QFormLayout;
    dnsForm->addRow(tr("Resolve host names:"), resolveCombo);
    dnsForm->addRow(tr("Resolve through:"), resolverCombo);
    dnsLayout->addLayout(dnsForm);
    const auto syncDns = [=] {
        resolverCombo->setEnabled(DnsResolve(resolveCombo->currentData().toInt()) == DnsResolve::Proxy);
    };
    connect(resolveCombo, &QComboBox::currentIndexChanged, &dlg, syncDns);
    syncDns();
    dnsBox->setEnabled(type != ActionType::Block);
    outer->addWidget(dnsBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    outer->addWidget(buttons);
    // A moderate starting width; the dialog can be resized to show more.
    dlg.resize(440, dlg.sizeHint().height());
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_interfaceId = combo->currentData().toString();
    m_skipIfInterfaceDown = !m_interfaceId.isEmpty() && ignore->isChecked();
    m_resolve = DnsResolve(resolveCombo->currentData().toInt());
    m_resolveProxyId = m_resolve == DnsResolve::Proxy ? resolverCombo->currentData().toInt() : 0;
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
    r.resolve = m_resolve;
    r.resolveProxyId = m_resolve == DnsResolve::Proxy ? m_resolveProxyId : 0;
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
