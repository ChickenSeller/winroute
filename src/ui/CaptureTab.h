#pragma once

#include <QWidget>

class Capture;
class CaptureModel;
class QItemSelection;
class QPlainTextEdit;
class QTableView;
class QTimer;
class QToolButton;

// The "Capture" tab: a list of captured connections on the left, a detail view
// (parsed HTTP, or a hex dump) on the right, and a toolbar to start/stop
// capture, clear it and export to PCAPNG / HAR. Capture is off by default; the
// toggle is persisted in QSettings ("capture/enabled").
class CaptureTab : public QWidget {
    Q_OBJECT
public:
    explicit CaptureTab(Capture *capture, QWidget *parent = nullptr);

private:
    void refresh();
    void onSelectionChanged();
    void renderDetail(quint64 id);
    void onToggled(bool on);
    void onClear();
    void onExport();
    void onFilter();
    void loadFilter();   // read the saved filter and apply it to the capture

    Capture *m_capture;
    CaptureModel *m_model;
    QTableView *m_view;
    QPlainTextEdit *m_detail;
    QToolButton *m_toggle;
    QTimer *m_timer;
    quint64 m_selectedId = 0;
};
