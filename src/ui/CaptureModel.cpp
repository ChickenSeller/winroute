#include "CaptureModel.h"
#include "Icons.h"

#include "core/Utils.h"

#include <QCoreApplication>
#include <QDateTime>

CaptureModel::CaptureModel(Capture *capture, QObject *parent)
    : QAbstractTableModel(parent), m_capture(capture)
{
}

int CaptureModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int CaptureModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant CaptureModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const CaptureSummary &r = m_rows[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColApp:
            return r.meta.appName;
        case ColTarget:
            return r.meta.target;
        case ColProto:
            return r.proto;
        case ColSent:
            return util::formatBytes(r.up);
        case ColReceived:
            return util::formatBytes(r.down);
        case ColTime:
            return r.meta.startMs ? QDateTime::fromMSecsSinceEpoch(r.meta.startMs).toString("HH:mm:ss") : QString();
        case ColStatus:
            if (r.truncated)
                return QCoreApplication::translate("CaptureModel", "Truncated");
            return r.open ? QCoreApplication::translate("CaptureModel", "Open")
                          : QCoreApplication::translate("CaptureModel", "Closed");
        }
    } else if (role == Qt::DecorationRole) {
        if (index.column() == ColProto && r.decrypted)
            return icons::lock();
    } else if (role == Qt::ToolTipRole) {
        if (index.column() == ColProto && r.decrypted)
            return QCoreApplication::translate("CaptureModel", "Decrypted from HTTPS");
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColSent || index.column() == ColReceived)
            return int(Qt::AlignRight | Qt::AlignVCenter);
    }
    return {};
}

QVariant CaptureModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColApp:
        return QCoreApplication::translate("CaptureModel", "Application");
    case ColTarget:
        return QCoreApplication::translate("CaptureModel", "Target");
    case ColProto:
        return QCoreApplication::translate("CaptureModel", "Protocol");
    case ColSent:
        return QCoreApplication::translate("CaptureModel", "Sent");
    case ColReceived:
        return QCoreApplication::translate("CaptureModel", "Received");
    case ColTime:
        return QCoreApplication::translate("CaptureModel", "Time");
    case ColStatus:
        return QCoreApplication::translate("CaptureModel", "Status");
    }
    return {};
}

void CaptureModel::refresh()
{
    QList<CaptureSummary> fresh = m_capture->summaries();
    if (fresh.size() != m_rows.size()) {
        beginResetModel();
        m_rows = std::move(fresh);
        endResetModel();
    } else {
        m_rows = std::move(fresh);
        if (!m_rows.isEmpty())
            emit dataChanged(index(0, 0), index(m_rows.size() - 1, ColCount - 1));
    }
}

quint64 CaptureModel::idAt(int row) const
{
    return (row >= 0 && row < m_rows.size()) ? m_rows[row].meta.id : 0;
}

int CaptureModel::rowOfId(quint64 id) const
{
    for (int i = 0; i < m_rows.size(); ++i)
        if (m_rows[i].meta.id == id)
            return i;
    return -1;
}
