#include "StatisticsWidget.h"
#include "core/Utils.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QHeaderView>
#include <QMenu>

enum { ColName, ColConnections, ColActive, ColSent, ColReceived, ColLast };

StatisticsWidget::StatisticsWidget(IconFn iconFn, QWidget *parent)
    : QTreeWidget(parent), m_iconFn(std::move(iconFn))
{
    setHeaderLabels({tr("Application / Target"), tr("Connections"), tr("Active"), tr("Sent"), tr("Received"), tr("Last Activity")});
    setRootIsDecorated(true);
    setUniformRowHeights(true);
    setSortingEnabled(true);
    sortByColumn(ColName, Qt::AscendingOrder);
    header()->setSectionResizeMode(ColName, QHeaderView::Stretch);
    header()->setStretchLastSection(false);
    for (int c = ColConnections; c <= ColLast; ++c)
        header()->resizeSection(c, c == ColLast ? 130 : 95);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu;
        menu.addAction(tr("&Copy"), this, &StatisticsWidget::copyAll);
        menu.addSeparator();
        menu.addAction(tr("&Reset Statistics"), this, &StatisticsWidget::reset);
        menu.exec(viewport()->mapToGlobal(pos));
    });
}

QTreeWidgetItem *StatisticsWidget::appItem(const ConnectionEvent &ev)
{
    const QString key = ev.appPath.isEmpty() ? ev.appName : ev.appPath.toLower();
    if (auto *item = m_apps.value(key))
        return item;
    auto *item = new QTreeWidgetItem(this);
    item->setText(ColName, ev.appName);
    item->setToolTip(ColName, ev.appPath);
    item->setIcon(ColName, m_iconFn(ev.appPath));
    m_apps.insert(key, item);
    return item;
}

QTreeWidgetItem *StatisticsWidget::targetItem(QTreeWidgetItem *app, const QString &target)
{
    for (int i = 0; i < app->childCount(); ++i)
        if (app->child(i)->text(ColName) == target)
            return app->child(i);
    auto *item = new QTreeWidgetItem(app);
    item->setText(ColName, target);
    return item;
}

void StatisticsWidget::refresh(QTreeWidgetItem *item)
{
    const Stat &s = m_stats[item];
    item->setText(ColConnections, QString::number(s.connections));
    item->setText(ColActive, QString::number(s.active));
    item->setText(ColSent, util::formatBytes(s.sent));
    item->setText(ColReceived, util::formatBytes(s.received));
    item->setText(ColLast, QDateTime::currentDateTime().toString("MM.dd HH:mm:ss"));
    for (int c = ColConnections; c <= ColReceived; ++c)
        item->setTextAlignment(c, Qt::AlignRight | Qt::AlignVCenter);
}

void StatisticsWidget::opened(const ConnectionEvent &ev)
{
    QTreeWidgetItem *app = appItem(ev);
    // Group targets by host; the port rarely matters for statistics.
    QString host = ev.target;
    const qsizetype colon = host.lastIndexOf(':');
    if (colon > 0 && !host.endsWith(']'))
        host = host.left(colon);
    if (host.startsWith('['))
        host = host.mid(1).chopped(1);
    QTreeWidgetItem *target = targetItem(app, host);
    for (QTreeWidgetItem *it : {app, target}) {
        Stat &s = m_stats[it];
        ++s.connections;
        ++s.active;
        refresh(it);
    }
    m_conns.insert(ev.id, Conn{app, target});
}

void StatisticsWidget::addBytes(Conn &c, qint64 sent, qint64 received)
{
    const qint64 ds = sent - c.sent, dr = received - c.received;
    c.sent = sent;
    c.received = received;
    if (ds == 0 && dr == 0)
        return;
    for (QTreeWidgetItem *it : {c.app, c.target}) {
        Stat &s = m_stats[it];
        s.sent += ds;
        s.received += dr;
        refresh(it);
    }
}

void StatisticsWidget::traffic(const QList<TrafficSample> &samples)
{
    for (const auto &smp : samples) {
        auto it = m_conns.find(smp.id);
        if (it != m_conns.end())
            addBytes(*it, smp.sent, smp.received);
    }
}

void StatisticsWidget::closed(quint64 id, qint64 sent, qint64 received)
{
    auto it = m_conns.find(id);
    if (it == m_conns.end())
        return;
    addBytes(*it, sent, received);
    for (QTreeWidgetItem *item : {it->app, it->target}) {
        Stat &s = m_stats[item];
        s.active = qMax(0, s.active - 1);
        refresh(item);
    }
    m_conns.erase(it);
}

void StatisticsWidget::reset()
{
    clear();
    m_apps.clear();
    m_stats.clear();
    m_conns.clear();
}

void StatisticsWidget::copyAll()
{
    QStringList lines;
    QStringList head;
    for (int c = 0; c < columnCount(); ++c)
        head << headerItem()->text(c);
    lines << head.join('\t');
    for (int i = 0; i < topLevelItemCount(); ++i) {
        auto *app = topLevelItem(i);
        QStringList cols;
        for (int c = 0; c < columnCount(); ++c)
            cols << app->text(c);
        lines << cols.join('\t');
        for (int j = 0; j < app->childCount(); ++j) {
            QStringList cc;
            for (int c = 0; c < columnCount(); ++c)
                cc << (c == 0 ? "    " : "") + app->child(j)->text(c);
            lines << cc.join('\t');
        }
    }
    QApplication::clipboard()->setText(lines.join('\n'));
}
