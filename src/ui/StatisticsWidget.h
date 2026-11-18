#pragma once

#include "core/Relay.h"

#include <QHash>
#include <QSet>
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
    void resetStatistics();
    void copyAll();

signals:
    // The running pids of the right-clicked application, to end (as a tree when
    // tree is true). Connected to the main window's End Process handler.
    void endProcessesRequested(const QSet<quint32> &pids, bool tree);

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
    void updateStatuses();                 // running / ended, for every app row
    QSet<quint32> runningPids(QTreeWidgetItem *app) const;

    IconFn m_iconFn;
    QHash<QString, QTreeWidgetItem *> m_apps;
    QHash<QTreeWidgetItem *, Stat> m_stats;
    QHash<QTreeWidgetItem *, QSet<quint32>> m_appPids;   // pids seen for each app row
    QHash<quint64, Conn> m_conns;
};
