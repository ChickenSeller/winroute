#include "ProxyServersDialog.h"
#include "ChainTypeDialog.h"
#include "Icons.h"
#include "ProxyCheckerDialog.h"
#include "ProxyServerDialog.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

enum { RoleId = Qt::UserRole, RoleIndex = Qt::UserRole + 1 };

// ---- ChainTree ---------------------------------------------------------------

ChainTree::ChainTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
}

void ChainTree::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->source() == proxySource || e->source() == this)
        e->acceptProposedAction();
    else
        e->ignore();
}

void ChainTree::dragMoveEvent(QDragMoveEvent *e)
{
    QTreeWidgetItem *item = itemAt(e->position().toPoint());
    if (!item) {
        e->ignore();
        return;
    }
    if (e->source() == this) {
        QTreeWidgetItem *dragged = currentItem();
        const bool sameChain = dragged && dragged->parent() && (item->parent() == dragged->parent() || item == dragged->parent());
        if (!sameChain) {
            e->ignore();
            return;
        }
    }
    e->acceptProposedAction();
}

void ChainTree::dropEvent(QDropEvent *e)
{
    QTreeWidgetItem *item = itemAt(e->position().toPoint());
    if (!item) {
        e->ignore();
        return;
    }
    QTreeWidgetItem *chainItem = item->parent() ? item->parent() : item;
    const int chainId = chainItem->data(0, RoleId).toInt();
    const int index = item->parent() ? item->data(0, RoleIndex).toInt() : chainItem->childCount();
    if (e->source() == proxySource) {
        if (auto *src = proxySource->currentItem(); src && onAddProxy)
            onAddProxy(chainId, src->data(0, RoleId).toInt(), index);
    } else if (e->source() == this) {
        QTreeWidgetItem *dragged = currentItem();
        if (dragged && dragged->parent() == chainItem && onMoveMember)
            onMoveMember(chainId, dragged->data(0, RoleIndex).toInt(), index);
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();   // the model is rebuilt by the callbacks, not by Qt's default move
}

// ---- ProxyServersDialog ------------------------------------------------------

ProxyServersDialog::ProxyServersDialog(const Profile &profile, QWidget *parent)
    : QDialog(parent), m_profile(profile)
{
    setWindowTitle(tr("Upstream Proxies"));
    resize(560, 300);
    auto *layout = new QVBoxLayout(this);

    auto *top = new QHBoxLayout;
    m_proxies = new QTreeWidget;
    m_proxies->setHeaderLabels({tr("Address"), tr("Port"), tr("Type")});
    m_proxies->setRootIsDecorated(false);
    m_proxies->setDragEnabled(true);
    m_proxies->setDragDropMode(QAbstractItemView::DragOnly);
    m_proxies->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_proxies->header()->setStretchLastSection(false);
    m_proxies->header()->resizeSection(1, 70);
    m_proxies->header()->resizeSection(2, 80);
    top->addWidget(m_proxies, 1);
    auto *pb = new QVBoxLayout;
    auto *add = new QPushButton(tr("Add..."));
    auto *edit = new QPushButton(tr("Edit..."));
    auto *remove = new QPushButton(tr("Remove"));
    auto *check = new QPushButton(tr("Check..."));
    for (auto *b : {add, edit, remove, check})
        pb->addWidget(b);
    pb->addStretch();
    top->addLayout(pb);
    layout->addLayout(top, 1);

    auto *chainRow = new QHBoxLayout;
    m_chainLabel = new QLabel(tr("You can chain several proxy servers together:"));
    m_chainsButton = new QPushButton(tr("Proxy Chains..."));
    m_chainsButton->setCheckable(true);
    chainRow->addWidget(m_chainLabel);
    chainRow->addWidget(m_chainsButton);
    chainRow->addStretch();
    layout->addLayout(chainRow);

    m_chainPanel = new QWidget;
    auto *cl = new QVBoxLayout(m_chainPanel);
    cl->setContentsMargins(0, 0, 0, 0);
    auto *chainTop = new QHBoxLayout;
    m_chains = new ChainTree;
    m_chains->setHeaderHidden(true);
    m_chains->proxySource = m_proxies;
    chainTop->addWidget(m_chains, 1);
    auto *cb = new QVBoxLayout;
    auto *create = new QPushButton(tr("Create"));
    auto *type = new QPushButton(tr("Type..."));
    auto *cremove = new QPushButton(tr("Remove"));
    for (auto *b : {create, type, cremove})
        cb->addWidget(b);
    cb->addStretch();
    chainTop->addLayout(cb);
    cl->addLayout(chainTop, 1);
    auto *hint = new QLabel(tr("Drag and Drop proxy servers within a chain to alter the order.\n"
                            "Use check boxes to enable/disable a proxy in a chain."));
    hint->setEnabled(false);
    cl->addWidget(hint);
    layout->addWidget(m_chainPanel, 1);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);

    connect(add, &QPushButton::clicked, this, &ProxyServersDialog::addProxy);
    connect(edit, &QPushButton::clicked, this, &ProxyServersDialog::editProxy);
    connect(remove, &QPushButton::clicked, this, &ProxyServersDialog::removeProxy);
    connect(check, &QPushButton::clicked, this, &ProxyServersDialog::checkProxy);
    connect(m_proxies, &QTreeWidget::itemDoubleClicked, this, &ProxyServersDialog::editProxy);
    connect(m_chainsButton, &QPushButton::toggled, this, &ProxyServersDialog::showChains);
    connect(create, &QPushButton::clicked, this, &ProxyServersDialog::createChain);
    connect(type, &QPushButton::clicked, this, &ProxyServersDialog::chainType);
    connect(cremove, &QPushButton::clicked, this, &ProxyServersDialog::removeChainOrMember);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_chains, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int) {
        if (!item->parent())
            return;
        const int chainId = item->parent()->data(0, RoleId).toInt();
        const int idx = item->data(0, RoleIndex).toInt();
        for (auto &c : m_profile.chains)
            if (c.id == chainId && idx < c.members.size())
                c.members[idx].enabled = item->checkState(0) == Qt::Checked;
    });
    connect(m_chains, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        if (item->parent())
            return;
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Proxy Chain"), tr("Chain name:"), QLineEdit::Normal, item->text(0), &ok);
        if (!ok || name.trimmed().isEmpty())
            return;
        for (auto &c : m_profile.chains)
            if (c.id == item->data(0, RoleId).toInt())
                c.name = name.trimmed();
        rebuildChains(item->data(0, RoleId).toInt());
    });
    m_chains->onAddProxy = [this](int chainId, int proxyId, int index) {
        for (auto &c : m_profile.chains)
            if (c.id == chainId)
                c.members.insert(qBound(0, index, int(c.members.size())), ChainMember{proxyId, true});
        rebuildChains(chainId);
    };
    m_chains->onMoveMember = [this](int chainId, int from, int to) {
        for (auto &c : m_profile.chains) {
            if (c.id != chainId || from >= c.members.size())
                continue;
            ChainMember m = c.members.takeAt(from);
            if (to > from)
                --to;
            c.members.insert(qBound(0, to, int(c.members.size())), m);
        }
        rebuildChains(chainId);
    };

    rebuildProxies();
    rebuildChains();
    showChains(!m_profile.chains.isEmpty());
}

void ProxyServersDialog::showChains(bool show)
{
    m_chainsButton->blockSignals(true);
    m_chainsButton->setChecked(show);
    m_chainsButton->blockSignals(false);
    m_chainPanel->setVisible(show);
    m_chainLabel->setText(show ? tr("Drag and Drop a proxy server from the list to add it into a chain.")
                               : tr("You can chain several proxy servers together:"));
    resize(width(), show ? 560 : 300);
}

void ProxyServersDialog::rebuildProxies(int selectId)
{
    m_proxies->clear();
    for (const ProxyServer &p : m_profile.proxies) {
        auto *item = new QTreeWidgetItem(m_proxies, {p.label.isEmpty() ? p.address : p.label, QString::number(p.port),
                                                     proxyTypeToString(p.type)});
        item->setIcon(0, icons::proxy());
        item->setData(0, RoleId, p.id);
        if (p.id == selectId)
            m_proxies->setCurrentItem(item);
    }
    if (!m_proxies->currentItem() && m_proxies->topLevelItemCount())
        m_proxies->setCurrentItem(m_proxies->topLevelItem(0));
}

void ProxyServersDialog::rebuildChains(int selectChainId)
{
    m_chains->blockSignals(true);
    m_chains->clear();
    for (const ProxyChain &c : m_profile.chains) {
        const QString typeName = c.type == ChainType::Simple ? QString() : c.type == ChainType::Redundancy ? tr(" (Redundancy)") : tr(" (Load Balancing)");
        auto *ci = new QTreeWidgetItem(m_chains, {c.name + typeName});
        ci->setIcon(0, icons::chain());
        ci->setData(0, RoleId, c.id);
        ci->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
        for (int i = 0; i < c.members.size(); ++i) {
            const ProxyServer *p = m_profile.proxyById(c.members[i].proxyId);
            auto *mi = new QTreeWidgetItem(ci, {p ? QString("%1  [%2]").arg(p->displayName(), proxyTypeToString(p->type))
                                                  : tr("<missing proxy %1>").arg(c.members[i].proxyId)});
            mi->setIcon(0, icons::proxy());
            mi->setData(0, RoleId, c.members[i].proxyId);
            mi->setData(0, RoleIndex, i);
            mi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled
                         | Qt::ItemIsDropEnabled);
            mi->setCheckState(0, c.members[i].enabled ? Qt::Checked : Qt::Unchecked);
        }
        ci->setExpanded(true);
        if (c.id == selectChainId)
            m_chains->setCurrentItem(ci);
    }
    m_chains->blockSignals(false);
}

int ProxyServersDialog::selectedProxyId() const
{
    auto *item = m_proxies->currentItem();
    return item ? item->data(0, RoleId).toInt() : 0;
}

int ProxyServersDialog::selectedChainId() const
{
    auto *item = m_chains->currentItem();
    if (!item)
        return 0;
    return (item->parent() ? item->parent() : item)->data(0, RoleId).toInt();
}

void ProxyServersDialog::addProxy()
{
    ProxyServer s;
    s.id = m_profile.nextId();
    ProxyServerDialog dlg(s, m_profile.options.httpProxiesSupport, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    s = dlg.server();
    m_profile.proxies.append(s);
    rebuildProxies(s.id);
    // Like Proxifier, offer to use a newly added (first) proxy by default.
    Rule &def = m_profile.rules.last();
    if (m_profile.proxies.size() == 1 && def.action.type == ActionType::Direct
        && QMessageBox::question(this, tr("Upstream Proxies"),
                                 tr("Do you want to use this proxy by default?\n\n"
                                         "The Default rule will be changed to use %1.\n"
                                         "You can change this later in Proxification Rules.")
                                     .arg(s.displayName()))
               == QMessageBox::Yes) {
        def.action = {ActionType::Proxy, s.id};
    }
    rebuildChains(selectedChainId());
}

void ProxyServersDialog::editProxy()
{
    const int id = selectedProxyId();
    for (auto &p : m_profile.proxies) {
        if (p.id != id)
            continue;
        ProxyServerDialog dlg(p, m_profile.options.httpProxiesSupport, this);
        if (dlg.exec() == QDialog::Accepted) {
            p = dlg.server();
            rebuildProxies(id);
            rebuildChains(selectedChainId());
        }
        return;
    }
}

void ProxyServersDialog::checkProxy()
{
    const ProxyServer *p = m_profile.proxyById(selectedProxyId());
    if (!p)
        return;
    ProxyCheckerDialog dlg({*p}, this);
    dlg.startTest();
    dlg.exec();
}

bool ProxyServersDialog::resolveRemoval(ActionType type, int id, const QString &name)
{
    QStringList ruleNames;
    QList<int> ruleIdx;
    for (int i = 0; i < m_profile.rules.size(); ++i) {
        const RuleAction &a = m_profile.rules[i].action;
        if (a.type == type && a.id == id) {
            ruleNames << m_profile.rules[i].name;
            ruleIdx << i;
        }
    }
    if (ruleIdx.isEmpty())
        return true;

    QDialog dlg(this);
    dlg.setWindowTitle(type == ActionType::Proxy ? tr("Proxy Removal") : tr("Chain Removal"));
    auto *l = new QGridLayout(&dlg);
    auto *icon = new QLabel;
    icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32, 32));
    l->addWidget(icon, 0, 0, Qt::AlignTop);
    l->addWidget(new QLabel(tr("%1 %2 is assigned as the Action for the following Rule(s):")
                                .arg(type == ActionType::Proxy ? tr("Proxy") : tr("Chain"), name)),
                 0, 1, 1, 2);
    auto *list = new QPlainTextEdit(ruleNames.join('\n'));
    list->setReadOnly(true);
    list->setMaximumHeight(70);
    l->addWidget(list, 1, 1, 1, 2);
    l->addWidget(new QLabel(tr("To proceed with the removal you can:")), 2, 1, 1, 2);
    auto *removeRules = new QRadioButton(tr("Remove the Rule(s)"));
    auto *newAction = new QRadioButton(tr("Select a new Action:"));
    auto *combo = new QComboBox;
    combo->addItem(tr("Direct"), QVariant::fromValue(int(ActionType::Direct) * 100000));
    combo->addItem(tr("Block"), QVariant::fromValue(int(ActionType::Block) * 100000));
    for (const ProxyServer &p : m_profile.proxies)
        if (!(type == ActionType::Proxy && p.id == id))
            combo->addItem(tr("Proxy %1 %2").arg(proxyTypeToString(p.type), p.displayName()), int(ActionType::Proxy) * 100000 + p.id);
    for (const ProxyChain &c : m_profile.chains)
        if (!(type == ActionType::Chain && c.id == id))
            combo->addItem(tr("Chain %1").arg(c.name), int(ActionType::Chain) * 100000 + c.id);
    const bool defaultUses = ruleIdx.contains(int(m_profile.rules.size()) - 1);
    removeRules->setChecked(!defaultUses);
    newAction->setChecked(defaultUses);
    if (defaultUses) {
        removeRules->setEnabled(false);
        auto *note = new QLabel(tr("The Default rule is used when no other rule matches, so it cannot be removed."));
        note->setWordWrap(true);
        l->addWidget(note, 5, 1, 1, 2);
    }
    l->addWidget(removeRules, 3, 1, 1, 2);
    l->addWidget(newAction, 4, 1);
    l->addWidget(combo, 4, 2);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    l->addWidget(bb, 6, 0, 1, 3);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    if (removeRules->isChecked()) {
        for (int i = int(ruleIdx.size()) - 1; i >= 0; --i)
            m_profile.rules.removeAt(ruleIdx[i]);
    } else {
        const int v = combo->currentData().toInt();
        const RuleAction a{ActionType(v / 100000), v % 100000};
        for (int i : ruleIdx)
            m_profile.rules[i].action = a;
    }
    return true;
}

void ProxyServersDialog::removeProxy()
{
    const int id = selectedProxyId();
    const ProxyServer *p = m_profile.proxyById(id);
    if (!p || !resolveRemoval(ActionType::Proxy, id, QString("%1:%2").arg(p->address).arg(p->port)))
        return;
    for (auto &c : m_profile.chains)
        c.members.removeIf([id](const ChainMember &m) { return m.proxyId == id; });
    m_profile.proxies.removeIf([id](const ProxyServer &s) { return s.id == id; });
    rebuildProxies();
    rebuildChains();
}

void ProxyServersDialog::createChain()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Proxy Chain"), tr("Chain name:"), QLineEdit::Normal,
                                               tr("Chain%1").arg(m_profile.chains.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    ProxyChain c;
    c.id = m_profile.nextId();
    c.name = name.trimmed();
    m_profile.chains.append(c);
    rebuildChains(c.id);
}

void ProxyServersDialog::chainType()
{
    const int id = selectedChainId();
    for (auto &c : m_profile.chains) {
        if (c.id != id)
            continue;
        ChainTypeDialog dlg(c, this);
        connect(&dlg, &ChainTypeDialog::resetFailuresRequested, this, &ProxyServersDialog::resetFailuresRequested);
        if (dlg.exec() == QDialog::Accepted) {
            dlg.apply(&c);
            rebuildChains(id);
        }
        return;
    }
}

void ProxyServersDialog::removeChainOrMember()
{
    QTreeWidgetItem *item = m_chains->currentItem();
    if (!item)
        return;
    const int chainId = selectedChainId();
    if (item->parent()) {
        const int idx = item->data(0, RoleIndex).toInt();
        for (auto &c : m_profile.chains)
            if (c.id == chainId && idx < c.members.size())
                c.members.removeAt(idx);
        rebuildChains(chainId);
        return;
    }
    const ProxyChain *c = m_profile.chainById(chainId);
    if (!c || !resolveRemoval(ActionType::Chain, chainId, c->name))
        return;
    m_profile.chains.removeIf([chainId](const ProxyChain &x) { return x.id == chainId; });
    rebuildChains();
}
