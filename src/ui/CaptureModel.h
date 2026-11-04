#pragma once

#include "core/Capture.h"

#include <QAbstractTableModel>
#include <QList>

// Rows of the "Capture" tab: one per connection the capture store knows about.
class CaptureModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColApp, ColTarget, ColProto, ColSent, ColReceived, ColTime, ColStatus, ColCount };

    explicit CaptureModel(Capture *capture, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;

    // Pulls a fresh snapshot: resets the model when connections were added or
    // dropped, otherwise just refreshes the live byte counts in place.
    void refresh();

    quint64 idAt(int row) const;
    int rowOfId(quint64 id) const;

private:
    Capture *m_capture;
    QList<CaptureSummary> m_rows;
};
