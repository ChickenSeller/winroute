#pragma once

#include "core/Engine.h"
#include "core/Profile.h"

#include <QHash>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>

class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QPlainTextEdit;
class QSortFilterProxyModel;
class QTabWidget;
class QTreeView;
class ConnectionsModel;
class DnsClient;
class StatisticsWidget;
class TrafficWidget;
class Updater;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Engine *engine, QWidget *parent = nullptr);
    ~MainWindow() override;

    void startEngine();
    void showAndActivate();
    void setStartMinimized(bool minimized) { m_startMinimized = minimized; }

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e) override;

private:
    // UI construction
    void createActions();
    void createMenus();
    void createToolBar();
    void createCentral();
    void createStatusBar();
    void createTray();

    // profiles
    static QString profilesDir();
    QString profilePath(const QString &name) const;
    QStringList profileNames() const;
    void initialProfile();
    bool loadProfile(const QString &name);
    void saveProfile();
    void applyProfile(const Profile &p);
    void rebuildProfileMenus();
    void newProfile();
    void saveProfileAs();
    void importProfile();
    void exportProfile();
    void manageProfiles();

    // profile settings
    void showProxyServers();
    void showRules();
    void showNameResolution();
    void showHttpProxies();
    void showServices();
    void showEncryption();
    void showProxyChecker();
    void showAbout();

    // connections
    QList<int> selectedRows() const;
    void disconnectSelected();
    void endProcessSelected();
    void createRuleForSelected();
    void copyConnections();

    // log
    void onLogEntry(int level, const QString &line);
    void setLogLevel(int level);
    void setLogFont(int size);

    // traffic
    void onOpened(const ConnectionEvent &ev);
    void onClosed(quint64 id, qint64 sent, qint64 received);
    void onTraffic(const QList<TrafficSample> &samples);
    void onSecondTick();
    void setRefreshRate(int ms);
    void probeLocalDns();

    // misc
    void changeLanguage(const QString &code);
    void setAutostart(bool on);
    static bool autostartEnabled();
    void updateTitle();
    void updateTrayIcon(double down, double up);

    Engine *m_engine;
    Profile m_profile;
    QString m_profileName;
    bool m_startMinimized = false;
    bool m_quitting = false;

    QTabWidget *m_tabs = nullptr;
    QTreeView *m_connView = nullptr;
    ConnectionsModel *m_connModel = nullptr;
    QSortFilterProxyModel *m_connSort = nullptr;
    TrafficWidget *m_traffic = nullptr;
    StatisticsWidget *m_stats = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QLabel *m_statusState = nullptr;
    QLabel *m_statusConns = nullptr;
    QLabel *m_statusRate = nullptr;
    QLabel *m_statusProfile = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_trayMenu = nullptr;
    QMenu *m_loadProfileMenu = nullptr;
    QMenu *m_trayProfileMenu = nullptr;
    QToolBar *m_toolBar = nullptr;

    QAction *m_actNewProfile, *m_actSaveAs, *m_actManage, *m_actImport, *m_actExport, *m_actAutostart, *m_actCloseWindow, *m_actExit;
    QAction *m_actProxyServers, *m_actRules, *m_actNameResolution, *m_actHttpProxies, *m_actServices, *m_actHandleDirect,
        *m_actLoopDetection, *m_actUdpProxy, *m_actEncryption;
    QAction *m_actDisconnect, *m_actEndProcess, *m_actCreateRule, *m_actCopyConn;
    QAction *m_actLogCopy, *m_actLogSelectAll, *m_actLogClear;
    QAction *m_actTrayIcon, *m_actTrayTraffic, *m_actToolbar, *m_actStatusBar;
    QAction *m_actChecker, *m_actAbout, *m_actCheckUpdates;
    Updater *m_updater = nullptr;
    QActionGroup *m_logLevelGroup = nullptr;
    QActionGroup *m_logFileGroup = nullptr;
    QActionGroup *m_refreshGroup = nullptr;
    QActionGroup *m_fontGroup = nullptr;

    int m_logLevel = 1;
    QTimer m_tickTimer;      // connection list refresh (refresh rate)
    QTimer m_secondTimer;    // traffic graph sampling
    QTimer m_dnsProbeTimer;
    DnsClient *m_dnsProbe = nullptr;
    QHash<quint64, QPair<qint64, qint64>> m_lastTotals;
    qint64 m_pendingIn = 0, m_pendingOut = 0;
    double m_maxRate = 1;
};
