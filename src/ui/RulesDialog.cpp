#include "RulesDialog.h"
#include "Icons.h"
#include "RuleDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

enum { ColName, ColApps, ColTargets, ColPorts, ColAction };

RulesDialog::RulesDialog(const Profile &profile, QWidget *parent)
    : QDialog(parent), m_profile(profile)
{
    setWindowTitle(tr("Proxification Rules"));
    resize(860, 440);
    auto *layout = new QVBoxLayout(this);

    auto *top = new QHBoxLayout;
    m_list = new QTreeWidget;
    m_list->setHeaderLabels({tr("Name"), tr("Applications"), tr("Target Hosts"), tr("Target Ports"), tr("Action")});
    m_list->setRootIsDecorated(false);
    m_list->setUniformRowHeights(true);
    m_list->setAllColumnsShowFocus(true);
    m_list->header()->resizeSection(ColName, 150);
    m_list->header()->resizeSection(ColApps, 200);
    m_list->header()->resizeSection(ColTargets, 200);
    m_list->header()->resizeSection(ColPorts, 90);
    top->addWidget(m_list, 1);
    auto *arrows = new QVBoxLayout;
    auto *up = new QToolButton;
    auto *down = new QToolButton;
    up->setIcon(icons::arrowUp());
    down->setIcon(icons::arrowDown());
    up->setToolTip(tr("Move up"));
    down->setToolTip(tr("Move down"));
    arrows->addStretch();
    arrows->addWidget(up);
    arrows->addWidget(down);
    arrows->addStretch();
    top->addLayout(arrows);
    layout->addLayout(top, 1);

    auto *buttons = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add..."));
    auto *clone = new QPushButton(tr("Clone"));
    auto *edit = new QPushButton(tr("Edit..."));
    auto *remove = new QPushButton(tr("Remove"));
    for (auto *b : {add, clone, edit, remove})
        buttons->addWidget(b);
    buttons->addStretch();
    layout->addLayout(buttons);
    auto *info = new QLabel(tr("Rules are processed from top to bottom. Use the arrows at the right to change the order.\n\n"
                            "Use check boxes to enable/disable a rule."));
    info->setEnabled(false);
    layout->addWidget(info);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);

    connect(add, &QPushButton::clicked, this, &RulesDialog::add);
    connect(clone, &QPushButton::clicked, this, &RulesDialog::clone);
    connect(edit, &QPushButton::clicked, this, &RulesDialog::edit);
    connect(remove, &QPushButton::clicked, this, &RulesDialog::remove);
    connect(up, &QToolButton::clicked, this, [this] { move(-1); });
    connect(down, &QToolButton::clicked, this, [this] { move(1); });
    connect(m_list, &QTreeWidget::itemDoubleClicked, this, &RulesDialog::edit);
    connect(m_list, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int col) {
        if (col != ColName)
            return;
        const int i = m_list->indexOfTopLevelItem(item);
        if (i >= 0 && i < m_profile.rules.size() - 1)
            m_profile.rules[i].enabled = item->checkState(ColName) == Qt::Checked;
    });
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    rebuild(0);
}

void RulesDialog::rebuild(int select)
{
    m_list->blockSignals(true);
    m_list->clear();
    const int last = int(m_profile.rules.size()) - 1;
    for (int i = 0; i <= last; ++i) {
        const Rule &r = m_profile.rules[i];
        auto *item = new QTreeWidgetItem(m_list, {r.name, r.applications.isEmpty() ? tr("Any") : r.applications,
                                                  r.targets.isEmpty() ? tr("Any") : r.targets, r.ports.isEmpty() ? tr("Any") : r.ports,
                                                  m_profile.actionDisplayName(r.action)});
        switch (r.action.type) {
        case ActionType::Direct: item->setIcon(ColAction, icons::direct()); break;
        case ActionType::Block: item->setIcon(ColAction, icons::block()); break;
        case ActionType::Proxy: item->setIcon(ColAction, icons::proxy()); break;
        case ActionType::Chain: item->setIcon(ColAction, icons::chain()); break;
        }
        for (int c = 0; c <= ColAction; ++c)
            item->setToolTip(c, item->text(c));
        if (i < last) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(ColName, r.enabled ? Qt::Checked : Qt::Unchecked);
            if (!r.enabled)
                for (int c = 0; c <= ColAction; ++c)
                    item->setForeground(c, palette().color(QPalette::Disabled, QPalette::Text));
        } else {
            QFont f = item->font(ColName);
            f.setBold(true);
            item->setFont(ColName, f);
        }
    }
    m_list->blockSignals(false);
    if (auto *item = m_list->topLevelItem(qBound(0, select, last)))
        m_list->setCurrentItem(item);
}

int RulesDialog::current() const
{
    return m_list->indexOfTopLevelItem(m_list->currentItem());
}

void RulesDialog::add()
{
    addRuleFor({});
}

void RulesDialog::addRuleFor(const QString &application)
{
    Rule r;
    r.name = application.isEmpty() ? tr("New") : application;
    r.applications = application;
    if (!m_profile.proxies.isEmpty())
        r.action = {ActionType::Proxy, m_profile.proxies.first().id};
    RuleDialog dlg(r, m_profile, false, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    // New rules go above the selected rule, never below Default.
    int at = current();
    if (at < 0 || at >= m_profile.rules.size() - 1)
        at = int(m_profile.rules.size()) - 1;
    m_profile.rules.insert(at, dlg.rule());
    rebuild(at);
}

void RulesDialog::clone()
{
    const int i = current();
    if (i < 0 || i >= m_profile.rules.size() - 1)
        return;
    Rule r = m_profile.rules[i];
    r.name += tr(" (copy)");
    m_profile.rules.insert(i + 1, r);
    rebuild(i + 1);
}

void RulesDialog::edit()
{
    const int i = current();
    if (i < 0)
        return;
    const bool isDefault = i == m_profile.rules.size() - 1;
    RuleDialog dlg(m_profile.rules[i], m_profile, isDefault, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_profile.rules[i] = dlg.rule();
        if (isDefault)
            m_profile.rules[i].enabled = true;
        rebuild(i);
    }
}

void RulesDialog::remove()
{
    const int i = current();
    if (i < 0)
        return;
    if (i == m_profile.rules.size() - 1) {
        QMessageBox::information(this, tr("Proxification Rules"),
                                 tr("The Default rule is used when all other rules do not match the connection "
                                 "parameters, so it cannot be removed."));
        return;
    }
    m_profile.rules.removeAt(i);
    rebuild(i);
}

void RulesDialog::move(int delta)
{
    const int i = current();
    const int j = i + delta;
    const int last = int(m_profile.rules.size()) - 1;
    if (i < 0 || i >= last || j < 0 || j >= last)
        return;
    m_profile.rules.swapItemsAt(i, j);
    rebuild(j);
}
