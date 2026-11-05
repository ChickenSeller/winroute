#pragma once

#include <QWidget>

#include "core/Capture.h"
class CaptureModel;
class CertAuthority;
class QCheckBox;
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
    explicit CaptureTab(Capture *capture, CertAuthority *ca, QWidget *parent = nullptr);
    // Whether the proxy gateway is running: its check boxes are disabled while
    // it is not (their state is kept for when it runs again).
    void setGatewayRunning(bool running);

private:
    void refresh();
    void onSelectionChanged();
    void renderDetail(quint64 id);
    void onToggled(bool on);
    void onDecryptToggled(Capture::Source source, QCheckBox *box, bool on);
    void onCertificate();   // the root-certificate dialog (generate / install)
    void onClear();
    void onExport();
    void onFilter();
    void loadFilter();   // read the saved filter and apply it to the capture

    Capture *m_capture;
    CertAuthority *m_ca;
    CaptureModel *m_model;
    QTableView *m_view;
    QPlainTextEdit *m_detail;
    QToolButton *m_toggle;
    QCheckBox *m_decryptLocal;     // decrypt HTTPS of this computer's connections
    QCheckBox *m_decryptGateway;   // ... and of the proxy gateway's
    QString m_gatewayTip, m_decryptGatewayTip;   // their tooltips while enabled
    int m_gatewayRunning = -1;                   // unknown until told
    QCheckBox *m_local;     // capture this computer's connections
    QCheckBox *m_gateway;   // capture the proxy gateway's connections
    QTimer *m_timer;
    quint64 m_selectedId = 0;
};
