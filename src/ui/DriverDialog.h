#pragma once

#include "core/NetFilterInterceptor.h"

#include <QDialog>
#include <QList>

class QComboBox;
class QLabel;
class QPushButton;
class Engine;

// Tools > Network Driver: choose the capture backend (NetFilter or WinDivert),
// and for NetFilter install, switch or uninstall its driver. The installer ships
// every NetFilter driver under <app>/drivers/<key>/; switching stops the engine,
// swaps the driver or backend, and starts it again.
class DriverDialog : public QDialog {
    Q_OBJECT
public:
    DriverDialog(Engine *engine, QWidget *parent = nullptr);

signals:
    void log(const QString &message);   // forwarded to the main window log

private:
    QString selectedBackend() const;   // "netfilter" or "windivert"
    void refresh();
    void onSelectionChanged();
    void apply();
    void uninstall();
    bool restartEngine(QString *error);

    Engine *m_engine;
    QList<NetFilterInterceptor::DriverChoice> m_drivers;
    QComboBox *m_backendCombo;
    QComboBox *m_combo;
    QLabel *m_driverLabel;
    QLabel *m_status;
    QLabel *m_note;
    QPushButton *m_apply;
    QPushButton *m_uninstall;
};
