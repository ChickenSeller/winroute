#include "ConnectionsModel.h"
#include "Icons.h"
#include "core/Utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPalette>
#include <algorithm>

ConnectionsModel::ConnectionsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ConnectionsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

int ConnectionsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QIcon ConnectionsModel::appIcon(const QString &path)
{
    auto it = m_icons.constFind(path);
    if (it != m_icons.cend())
        return *it;
    QIcon icon = path.isEmpty() ? m_iconProvider.icon(QFileIconProvider::Computer) : m_iconProvider.icon(QFileInfo(path));
    if (icon.isNull())
        icon = m_iconProvider.icon(QFileIconProvider::File);
    m_icons.insert(path, icon);
    return icon;
}

// Row statuses are untranslated codes ("Connecting", "Open", "Closed", "Error : <message>").
static QString timeStatus(const QString &status, qint64 startMs, qint64 closedMs)
{
    if (status == "Open") {
        if (closedMs)
            return QCoreApplication::translate("ConnectionsModel", "Closed");
        return util::formatDuration((QDateTime::currentMSecsSinceEpoch() - startMs) / 1000);
    }
    if (status == "Connecting")
        return QCoreApplication::translate("ConnectionsModel", "Connecting");
    if (status == "Closed")
        return QCoreApplication::translate("ConnectionsModel", "Closed");
    if (status.startsWith("Error : "))
        return QCoreApplication::translate("ConnectionsModel", "Error : %1").arg(status.mid(8));
    return status;
}

QVariant ConnectionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        switch (index.column()) {
        case ColApplication:
            if (role == Qt::ToolTipRole)
                return tr("%1 (pid %2)").arg(r.ev.appPath.isEmpty() ? r.ev.appName : r.ev.appPath).arg(r.ev.pid);
            return r.ev.appName;
        case ColTarget: return r.ev.target;
        case ColTimeStatus: return timeStatus(r.status, r.ev.startMs, r.closedMs);
        case ColRuleProxy: return QString("%1 : %2").arg(r.ev.ruleName, r.ev.actionText);
        case ColSent: return util::formatBytes(r.sent);
        case ColReceived: return util::formatBytes(r.received);
        }
    } else if (role == Qt::DecorationRole && index.column() == ColApplication) {
        return const_cast<ConnectionsModel *>(this)->appIcon(r.ev.appPath);
    } else if (role == Qt::DecorationRole && index.column() == ColRuleProxy) {
        switch (r.ev.actionType) {
        case ActionType::Direct: return icons::direct();
        case ActionType::Block: return icons::block();
        case ActionType::Chain: return icons::chain();
        case ActionType::Proxy: return icons::proxy();
        }
    } else if (role == Qt::ForegroundRole) {
        if (r.status.startsWith("Error"))
            return QColor(200, 30, 30);
        if (r.closedMs)
            return QGuiApplication::palette().color(QPalette::Disabled, QPalette::Text);
    } else if (role == Qt::TextAlignmentRole && (index.column() == ColSent || index.column() == ColReceived)) {
        return int(Qt::AlignRight | Qt::AlignVCenter);
    } else if (role == Qt::UserRole) {
        switch (index.column()) {
        case ColTimeStatus: return r.ev.startMs;
        case ColSent: return r.sent;
        case ColReceived: return r.received;
        default: return data(index, Qt::DisplayRole).toString().toLower();
        }
    }
    return {};
}

QVariant ConnectionsModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColApplication: return tr("Application");
    case ColTarget: return tr("Target");
    case ColTimeStatus: return tr("Time/Status");
    case ColRuleProxy: return tr("Rule : Proxy");
    case ColSent: return tr("Sent");
    case ColReceived: return tr("Received");
    }
    return {};
}

void ConnectionsModel::sort(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;
    if (column < 0)
        return;
    emit layoutAboutToBeChanged();
    std::stable_sort(m_rows.begin(), m_rows.end(), [&](const Row &a, const Row &b) {
        auto key = [&](const Row &r) -> QVariant {
            switch (column) {
            case ColApplication: return r.ev.appName.toLower();
            case ColTarget: return r.ev.target.toLower();
            case ColTimeStatus: return r.ev.startMs;
            case ColRuleProxy: return r.ev.ruleName.toLower();
            case ColSent: return r.sent;
            default: return r.received;
            }
        };
        const auto ord = QVariant::compare(key(a), key(b));
        return order == Qt::AscendingOrder ? ord == QPartialOrdering::Less : ord == QPartialOrdering::Greater;
    });
    reindex();
    emit layoutChanged();
}

void ConnectionsModel::reindex()
{
    m_index.clear();
    for (int i = 0; i < m_rows.size(); ++i)
        m_index.insert(m_rows[i].ev.id, i);
}

int ConnectionsModel::rowOf(quint64 id) const
{
    return m_index.value(id, -1);
}

void ConnectionsModel::opened(const ConnectionEvent &ev)
{
    if (rowOf(ev.id) >= 0)
        return;
    const int row = int(m_rows.size());
    beginInsertRows({}, row, row);
    Row r;
    r.ev = ev;
    r.status = "Connecting";
    m_rows.append(r);
    m_index.insert(ev.id, row);
    endInsertRows();
}

void ConnectionsModel::statusChanged(quint64 id, const QString &status)
{
    const int row = rowOf(id);
    if (row < 0)
        return;
    m_rows[row].status = status;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}

void ConnectionsModel::closed(quint64 id, qint64 sent, qint64 received)
{
    const int row = rowOf(id);
    if (row < 0)
        return;
    Row &r = m_rows[row];
    r.sent = sent;
    r.received = received;
    r.closedMs = QDateTime::currentMSecsSinceEpoch();
    if (r.status == "Connecting")
        r.status = "Closed";
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}

void ConnectionsModel::traffic(const QList<TrafficSample> &samples)
{
    for (const auto &s : samples) {
        const int row = rowOf(s.id);
        if (row < 0)
            continue;
        m_rows[row].sent = s.sent;
        m_rows[row].received = s.received;
        emit dataChanged(index(row, ColSent), index(row, ColReceived));
    }
}

void ConnectionsModel::tick(int keepMs)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int i = int(m_rows.size()) - 1; i >= 0; --i) {
        const Row &r = m_rows[i];
        // Failed connections stay a little longer so the error can be read.
        const int keep = r.status.startsWith("Error") ? keepMs * 3 : keepMs;
        if (r.closedMs && now - r.closedMs > keep) {
            beginRemoveRows({}, i, i);
            m_rows.removeAt(i);
            endRemoveRows();
        }
    }
    reindex();
    if (!m_rows.isEmpty())
        emit dataChanged(index(0, ColTimeStatus), index(int(m_rows.size()) - 1, ColTimeStatus));
}

quint64 ConnectionsModel::idAt(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows[row].ev.id : 0;
}

quint32 ConnectionsModel::pidAt(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows[row].ev.pid : 0;
}

int ConnectionsModel::activeCount() const
{
    return int(std::count_if(m_rows.cbegin(), m_rows.cend(), [](const Row &r) { return !r.closedMs; }));
}

QString ConnectionsModel::rowText(int row) const
{
    QStringList cols;
    for (int c = 0; c < ColCount; ++c)
        cols << data(index(row, c), Qt::DisplayRole).toString();
    return cols.join('\t');
}
