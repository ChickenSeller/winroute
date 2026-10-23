#pragma once

#include "core/Relay.h"

#include <QHash>
#include <QTreeWidget>
#include <functional>

// "Statistics" tab: per application totals, expandable to per target host.
class StatisticsWidget : public QTreeWidget {
    Q_OBJECT
public:
    using IconFn = std::function<QIcon(const QString &path)>;

    explicit StatisticsWidget(IconFn iconFn, QWidget *parent = nullptr);

    void opened(const ConnectionEvent &ev);
    void closed(quint64 id, qint64 sent, qint64 received);
    void traffic(const QList<TrafficSample> &samples);
    void reset();
    void copyAll();

private:
    struct Stat {
        int connections = 0;
        int active = 0;
        qint64 sent = 0;
        qint64 received = 0;
    };
    struct Conn {
        QTreeWidgetItem *app;
        QTreeWidgetItem *target;
        qint64 sent = 0;
        qint64 received = 0;
    };
    QTreeWidgetItem *appItem(const ConnectionEvent &ev);
    QTreeWidgetItem *targetItem(QTreeWidgetItem *app, const QString &target);
    void addBytes(Conn &c, qint64 sent, qint64 received);
    void refresh(QTreeWidgetItem *item);

    IconFn m_iconFn;
    QHash<QString, QTreeWidgetItem *> m_apps;
    QHash<QTreeWidgetItem *, Stat> m_stats;
    QHash<quint64, Conn> m_conns;
};
