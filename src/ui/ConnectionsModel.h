#pragma once

#include "core/Relay.h"

#include <QAbstractTableModel>
#include <QFileIconProvider>
#include <QHash>
#include <QIcon>

// Rows of the "Connections" tab. Closed connections stay visible (greyed)
// for a few seconds, like in Proxifier.
class ConnectionsModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColApplication, ColTarget, ColTimeStatus, ColRuleProxy, ColSent, ColReceived, ColCount };

    explicit ConnectionsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    void sort(int column, Qt::SortOrder order) override;

    void opened(const ConnectionEvent &ev);
    void statusChanged(quint64 id, const QString &status);
    void closed(quint64 id, qint64 sent, qint64 received);
    void traffic(const QList<TrafficSample> &samples);

    // Refreshes elapsed times and drops rows closed for longer than `keepMs`.
    void tick(int keepMs);

    // Drops the direct (non-proxied) connections from the display; used when
    // "Handle Direct Connections" is turned off. The connections themselves are
    // not touched, only hidden.
    void removeDirect();

    quint64 idAt(int row) const;
    quint32 pidAt(int row) const;
    int activeCount() const;
    QString rowText(int row) const;

    QIcon appIcon(const QString &path);

private:
    struct Row {
        ConnectionEvent ev;
        QString status;    // "Connecting", "Open", "Error : ...", "Closed"
        qint64 sent = 0;
        qint64 received = 0;
        qint64 closedMs = 0;
    };
    int rowOf(quint64 id) const;
    void reindex();

    QList<Row> m_rows;
    QHash<quint64, int> m_index;
    QHash<QString, QIcon> m_icons;
    QFileIconProvider m_iconProvider;
    int m_sortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
