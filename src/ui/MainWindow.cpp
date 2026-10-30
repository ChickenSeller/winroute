#include "MainWindow.h"
#include "AdvancedDialogs.h"
#include "ConnectionsModel.h"
#include "Icons.h"
#include "Language.h"
#include "NameResolutionDialog.h"
#include "ProxyCheckerDialog.h"
#include "ProxyGatewayDialog.h"
#include "ProxyServersDialog.h"
#include "core/ProxyGateway.h"
#include "RulesDialog.h"
#include "StatisticsWidget.h"
#include "DriverDialog.h"
#include "TrafficWidget.h"
#include "Updater.h"
#include "UsageStats.h"
#include "core/DnsClient.h"
#include "core/Logger.h"
#include "core/NetFilterInterceptor.h"
#include "core/WinDivertInterceptor.h"
#include "core/ProcessInfo.h"
#include "core/Utils.h"

#include <QCoreApplication>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

static const char *kTaskName = "WinRoute";

MainWindow::MainWindow(Engine *engine, QWidget *parent)
    : QMainWindow(parent), m_engine(engine)
{
    setWindowIcon(icons::app(true));
    resize(1100, 720);

    createActions();
    createMenus();
    createToolBar();
    createCentral();
    createStatusBar();
    createTray();

    connect(&Logger::instance(), &Logger::entryAdded, this, &MainWindow::onLogEntry, Qt::QueuedConnection);
    logNormal(tr("Welcome to WinRoute v%1").arg(QApplication::applicationVersion()));
    Relay *relay = m_engine->relay();
    connect(relay, &Relay::opened, this, &MainWindow::onOpened);
    connect(relay, &Relay::statusChanged, m_connModel, &ConnectionsModel::statusChanged);
    connect(relay, &Relay::closed, this, &MainWindow::onClosed);
    connect(relay, &Relay::traffic, this, &MainWindow::onTraffic);
    connect(&m_tickTimer, &QTimer::timeout, this, [this] {
        m_connModel->tick(3000);
        m_statusConns->setText(tr("%1 active connections").arg(m_connModel->activeCount()));
    });
    connect(&m_secondTimer, &QTimer::timeout, this, &MainWindow::onSecondTick);
    m_secondTimer.start(1000);

    QSettings s = util::appSettings();
    restoreGeometry(s.value("window/geometry").toByteArray());
    restoreState(s.value("window/state").toByteArray());
    setLogLevel(s.value("log/level", 1).toInt());
    setLogFont(s.value("log/font", 9).toInt());
    Logger::instance().setFileLevel(s.value("log/fileLevel", -1).toInt());
    for (QAction *a : m_logFileGroup->actions())
        a->setChecked(a->data().toInt() == Logger::instance().fileLevel());
    setRefreshRate(s.value("view/refresh", 1000).toInt());
    m_actTrayIcon->setChecked(s.value("view/trayIcon", true).toBool());
    m_tray->setVisible(m_actTrayIcon->isChecked());

    initialProfile();

    // Start the inbound proxy service now if it is enabled in WinRoute.ini.
    QString gwError;
    if (!m_engine->applyGatewayConfig(ProxyGateway::loadConfig(), &gwError))
        logError(tr("Proxy gateway: %1").arg(gwError));

    m_dnsProbe = new DnsClient(this);
    m_dnsProbe->bind(0);
    connect(&m_dnsProbeTimer, &QTimer::timeout, this, &MainWindow::probeLocalDns);
    m_dnsProbeTimer.start(60000);
}

MainWindow::~MainWindow() = default;

// ---- construction ------------------------------------------------------------

void MainWindow::createActions()
{
    auto act = [this](const QString &text, auto slot, const QKeySequence &key = {}, const QIcon &icon = {}) {
        auto *a = new QAction(icon, text, this);
        if (!key.isEmpty())
            a->setShortcut(key);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    m_actNewProfile = act(tr("&New Profile"), &MainWindow::newProfile, QKeySequence("Ctrl+N"));
    m_actSaveAs = act(tr("&Save Profile As..."), &MainWindow::saveProfileAs, QKeySequence("Ctrl+S"));
    m_actManage = act(tr("&Manage Profiles..."), &MainWindow::manageProfiles, QKeySequence("Ctrl+M"));
    m_actImport = act(tr("&Import Profile..."), &MainWindow::importProfile, QKeySequence("Ctrl+O"));
    m_actExport = act(tr("&Export Profile..."), &MainWindow::exportProfile);
    m_actAutostart = new QAction(tr("&Autostart"), this);
    m_actAutostart->setCheckable(true);
    m_actAutostart->setChecked(autostartEnabled());
    m_actAutostart->setStatusTip(tr("Start WinRoute automatically when Windows starts for the current user"));
    connect(m_actAutostart, &QAction::toggled, this, &MainWindow::setAutostart);
    m_actCloseWindow = act(tr("&Close WinRoute Window"), &QWidget::hide);
    m_actExit = act(tr("E&xit"), [this] {
        m_quitting = true;
        close();
    });

    m_actProxyServers = act(tr("&Upstream Proxies..."), &MainWindow::showProxyServers, {}, icons::proxyServers());
    m_actProxyServers->setStatusTip(tr("Configure upstream proxy servers"));
    m_actRules = act(tr("Proxification &Rules..."), &MainWindow::showRules, {}, icons::rules());
    m_actRules->setStatusTip(tr("Edit proxification rules"));
    m_actNameResolution = act(tr("&Name Resolution..."), &MainWindow::showNameResolution, {}, icons::nameResolution());
    m_actNameResolution->setStatusTip(tr("Change the way WinRoute handles host name resolution"));
    m_actHttpProxies = act(tr("&HTTP Proxy Servers..."), &MainWindow::showHttpProxies);
    m_actServices = act(tr("&Services and Other Users..."), &MainWindow::showServices);
    m_actHandleDirect = new QAction(tr("Handle &Direct Connections"), this);
    m_actHandleDirect->setCheckable(true);
    m_actHandleDirect->setStatusTip(tr("Display and handle connections that are established directly"));
    connect(m_actHandleDirect, &QAction::triggered, this, [this](bool on) {
        Profile p = m_profile;
        p.options.handleDirectConnections = on;
        applyProfile(p);
        // New connections already follow the new setting; reflect it in the list
        // at once by dropping the direct connections that are still shown. (They
        // keep running; established connections can't change how they are routed.)
        if (!on)
            m_connModel->removeDirect();
    });
    m_actLoopDetection = new QAction(tr("Connection &Loop Detection"), this);
    m_actLoopDetection->setCheckable(true);
    m_actLoopDetection->setStatusTip(tr("Connect directly to proxy servers to prevent infinite connection loops"));
    connect(m_actLoopDetection, &QAction::triggered, this, [this](bool on) {
        Profile p = m_profile;
        p.options.loopDetection = on;
        applyProfile(p);
    });
    m_actUdpProxy = new QAction(tr("Route &UDP through SOCKS5"), this);
    m_actUdpProxy->setCheckable(true);
    m_actUdpProxy->setStatusTip(tr("Tunnel UDP through the proxy when a rule selects a SOCKS5 proxy (otherwise UDP goes directly)"));
    connect(m_actUdpProxy, &QAction::triggered, this, [this](bool on) {
        Profile p = m_profile;
        p.options.udpMode = on ? "mode_proxy" : "mode_bypass";
        applyProfile(p);
    });
    m_actLogDns = new QAction(tr("Log &DNS Activity"), this);
    m_actLogDns->setCheckable(true);
    m_actLogDns->setStatusTip(tr("Log DNS queries and replies: the server, the name asked, the answer, the program and whether it is resolved through the proxy"));
    connect(m_actLogDns, &QAction::triggered, this, [this](bool on) {
        Profile p = m_profile;
        p.options.logDns = on;
        applyProfile(p);
    });
    m_actEncryption = act(tr("&Passwords Encryption..."), &MainWindow::showEncryption);

    m_actDisconnect = act(tr("&Disconnect"), &MainWindow::disconnectSelected, QKeySequence::Delete, icons::disconnect());
    m_actDisconnect->setStatusTip(tr("Disconnect selected connection"));
    m_actDisconnect->setIconText(tr("Disconnect"));   // toolbar text without the "(&D)" accelerator
    m_actEndProcess = act(tr("&End Process"), &MainWindow::endProcessSelected, {}, icons::endProcess());
    m_actEndProcess->setStatusTip(tr("Terminate parent process of the selected connection"));
    m_actEndProcess->setIconText(tr("End Process"));
    m_actCreateRule = act(tr("&Create Rule for this Application..."), &MainWindow::createRuleForSelected, {}, icons::rules());
    m_actCopyConn = act(tr("&Copy"), &MainWindow::copyConnections, QKeySequence::Copy);
    m_actCopyConn->setShortcutContext(Qt::WidgetShortcut);

    m_actLogCopy = act(tr("&Copy"), [this] { m_log->copy(); });
    m_actLogSelectAll = act(tr("Select A&ll"), [this] { m_log->selectAll(); });
    m_actLogClear = act(tr("Clea&r"), [this] { m_log->clear(); });

    m_actTrayIcon = new QAction(tr("Icon in the System &Tray"), this);
    m_actTrayIcon->setCheckable(true);
    m_actTrayIcon->setStatusTip(tr("Show WinRoute icon in the System Tray and hide the main window when closed"));
    connect(m_actTrayIcon, &QAction::toggled, this, [this](bool on) {
        m_tray->setVisible(on);
        util::appSettings().setValue("view/trayIcon", on);
    });
    m_actToolbar = new QAction(tr("&Toolbar"), this);
    m_actToolbar->setCheckable(true);
    m_actToolbar->setChecked(true);
    m_actStatusBar = new QAction(tr("&Status Bar"), this);
    m_actStatusBar->setCheckable(true);
    m_actStatusBar->setChecked(true);

    m_actChecker = act(tr("Proxy &Checker..."), &MainWindow::showProxyChecker, {}, icons::checker());
    m_actChecker->setStatusTip(tr("Test proxy servers"));
    m_actDriver = act(tr("&Network Driver..."), [this] {
        DriverDialog dlg(m_engine, this);
        connect(&dlg, &DriverDialog::log, this, [this](const QString &m) { logNormal(m); });
        dlg.exec();
        updateDriverStatus();
    });
    m_actDriver->setStatusTip(tr("Install, switch or uninstall the network driver"));
    m_actAbout = act(tr("&About WinRoute..."), &MainWindow::showAbout);

    m_updater = new Updater(this);
    m_actCheckUpdates = act(tr("Check for &Updates..."), [this] { m_updater->checkNow(); });
    m_actCheckUpdatesProxy = act(tr("Check for Updates via &Proxy..."), [this] { checkForUpdatesViaProxy(); });
    m_actCheckUpdatesProxy->setStatusTip(tr("Check for and download updates through one of this profile's proxies"));
    connect(m_updater, &Updater::notify, this, [this](const QString &title, const QString &message) {
        if (m_tray->isVisible())
            m_tray->showMessage(title, message);
        else
            logNormal(message);
    });
    connect(m_updater, &Updater::quitRequested, this, [this] {
        m_quitting = true;
        close();
    });
    QTimer::singleShot(5000, m_updater, &Updater::checkAtStartup);
}

void MainWindow::createMenus()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_actNewProfile);
    m_loadProfileMenu = file->addMenu(tr("&Load Profile"));
    file->addAction(m_actSaveAs);
    file->addSeparator();
    file->addAction(m_actManage);
    file->addAction(m_actImport);
    file->addAction(m_actExport);
    file->addSeparator();
    file->addAction(m_actAutostart);
    file->addSeparator();
    file->addAction(m_actCloseWindow);
    file->addAction(m_actExit);

    QMenu *profile = menuBar()->addMenu(tr("&Profile"));
    profile->addAction(m_actProxyServers);
    profile->addAction(m_actRules);
    profile->addAction(m_actNameResolution);
    profile->addSeparator();
    QMenu *advanced = profile->addMenu(tr("&Advanced"));
    advanced->addAction(m_actHttpProxies);
    advanced->addAction(m_actServices);
    advanced->addAction(m_actHandleDirect);
    advanced->addAction(m_actLoopDetection);
    advanced->addAction(m_actUdpProxy);
    advanced->addAction(m_actLogDns);
    advanced->addAction(m_actEncryption);

    QMenu *log = menuBar()->addMenu(tr("&Log"));
    log->addAction(m_actLogCopy);
    log->addAction(m_actLogSelectAll);
    log->addAction(m_actLogClear);
    log->addSeparator();
    QMenu *level = log->addMenu(tr("L&og Level"));
    m_logLevelGroup = new QActionGroup(this);
    const QList<QPair<QString, int>> levels{{tr("&Errors Only"), 0}, {tr("&Normal"), 1}, {tr("&Verbose"), 2}, {tr("&Debug"), 3}};
    for (const auto &[name, v] : levels) {
        QAction *a = level->addAction(name);
        a->setCheckable(true);
        a->setData(v);
        m_logLevelGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, v] { setLogLevel(v); });
    }
    QMenu *fileLevel = log->addMenu(tr("Log to &File"));
    m_logFileGroup = new QActionGroup(this);
    const QList<QPair<QString, int>> fileLevels{{tr("&Disabled"), -1}, {tr("&Errors Only"), 0}, {tr("&Normal"), 1}, {tr("&Verbose"), 2}, {tr("De&bug"), 3}};
    for (const auto &[name, v] : fileLevels) {
        QAction *a = fileLevel->addAction(name);
        a->setCheckable(true);
        a->setData(v);
        m_logFileGroup->addAction(a);
        connect(a, &QAction::triggered, this, [v] {
            Logger::instance().setFileLevel(v);
            util::appSettings().setValue("log/fileLevel", v);
            if (v >= 0)
                logNormal(tr("Writing log to ") + QDir::toNativeSeparators(Logger::instance().filePath()));
        });
    }
    QMenu *font = log->addMenu(tr("&Font Size"));
    m_fontGroup = new QActionGroup(this);
    const QList<QPair<QString, int>> sizes{{tr("&Small"), 8}, {tr("&Medium"), 9}, {tr("&Large"), 11}};
    for (const auto &[name, v] : sizes) {
        QAction *a = font->addAction(name);
        a->setCheckable(true);
        a->setData(v);
        m_fontGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, v] { setLogFont(v); });
    }

    QMenu *view = menuBar()->addMenu(tr("&View"));
    view->addAction(m_actToolbar);
    view->addAction(m_actStatusBar);
    view->addSeparator();
    QMenu *refresh = view->addMenu(tr("&Refresh Rate"));
    m_refreshGroup = new QActionGroup(this);
    const QList<QPair<QString, int>> rates{{tr("&Fast"), 500}, {tr("&Normal"), 1000}, {tr("&Slow"), 2000}};
    for (const auto &[name, v] : rates) {
        QAction *a = refresh->addAction(name);
        a->setCheckable(true);
        a->setData(v);
        m_refreshGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, v] { setRefreshRate(v); });
    }
    view->addSeparator();
    view->addAction(m_actTrayIcon);
    view->addSeparator();
    QMenu *language = view->addMenu(tr("&Language"));
    auto *languageGroup = new QActionGroup(this);
    for (const i18n::Language &l : i18n::available()) {
        QAction *a = language->addAction(l.nativeName);
        a->setCheckable(true);
        a->setChecked(l.code == i18n::configured());
        languageGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, code = l.code] { changeLanguage(code); });
    }

    QMenu *tools = menuBar()->addMenu(tr("&Tools"));
    tools->addAction(m_actChecker);
    tools->addAction(m_actDriver);

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(m_actCheckUpdates);
    help->addAction(m_actCheckUpdatesProxy);
    QMenu *updates = help->addMenu(tr("Automatic U&pdates"));
    auto *updateGroup = new QActionGroup(this);
    const QList<QPair<QString, Updater::Mode>> updateModes = {
        {tr("Install All Updates Automatically"), Updater::Mode::Automatic},
        {tr("Install Required Updates Automatically"), Updater::Mode::RequiredOnly},
        {tr("Ask Before Updating"), Updater::Mode::Ask},
        {tr("Never Check"), Updater::Mode::Never},
    };
    for (const auto &[name, mode] : updateModes) {
        QAction *a = updates->addAction(name);
        a->setCheckable(true);
        a->setChecked(mode == Updater::mode());
        updateGroup->addAction(a);
        connect(a, &QAction::triggered, this, [mode] { Updater::setMode(mode); });
    }
    if (UsageStats::isAvailable()) {
        QAction *stats = help->addAction(tr("Send Anonymous Usage &Statistics"));
        stats->setStatusTip(tr("Send anonymous usage statistics to help improve WinRoute (no personal data)."));
        stats->setCheckable(true);
        stats->setChecked(UsageStats::isEnabled());
        connect(stats, &QAction::toggled, this, [](bool on) { UsageStats::setEnabled(on); });
    }
    help->addSeparator();
    help->addAction(m_actAbout);
}

void MainWindow::createToolBar()
{
    m_toolBar = addToolBar(tr("Toolbar"));
    m_toolBar->setObjectName("mainToolBar");
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(32, 32));
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto *proxyServers = new QAction(icons::proxyServers(), tr("Upstream Proxies"), this);
    proxyServers->setToolTip(tr("Set up the upstream SOCKS/HTTPS proxies this machine routes traffic through, and chain them together."));
    auto *rules = new QAction(icons::rules(), tr("Proxification Rules"), this);
    rules->setToolTip(tr("Decide how each connection is handled — direct, a proxy, a chain or blocked — by target, port and application."));
    auto *dns = new QAction(icons::nameResolution(), tr("Name Resolution"), this);
    dns->setToolTip(tr("Choose how host names are resolved: locally or through the proxy."));
    auto *checker = new QAction(icons::checker(), tr("Proxy Checker"), this);
    checker->setToolTip(tr("Test whether proxy servers are reachable and how fast they respond."));
    auto *proxyServer = new QAction(icons::proxyService(), tr("Proxy Gateway"), this);
    proxyServer->setToolTip(tr("Let other machines use this computer as an HTTP/HTTPS or SOCKS5 proxy to reach the network."));
    proxyServer->setStatusTip(tr("Let other machines reach the network through this one"));
    connect(proxyServers, &QAction::triggered, this, &MainWindow::showProxyServers);
    connect(rules, &QAction::triggered, this, &MainWindow::showRules);
    connect(dns, &QAction::triggered, this, &MainWindow::showNameResolution);
    connect(checker, &QAction::triggered, this, &MainWindow::showProxyChecker);
    connect(proxyServer, &QAction::triggered, this, &MainWindow::showProxyServerSettings);
    m_toolBar->addAction(proxyServers);
    m_toolBar->addAction(rules);
    m_toolBar->addAction(dns);
    m_toolBar->addSeparator();
    m_toolBar->addAction(proxyServer);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actDisconnect);
    m_toolBar->addAction(m_actEndProcess);
    m_toolBar->addSeparator();
    m_toolBar->addAction(checker);
    connect(m_actToolbar, &QAction::toggled, m_toolBar, &QToolBar::setVisible);
}

void MainWindow::createCentral()
{
    auto *splitter = new QSplitter(Qt::Vertical);
    m_tabs = new QTabWidget;
    m_tabs->setDocumentMode(true);

    m_connModel = new ConnectionsModel(this);
    m_connSort = new QSortFilterProxyModel(this);
    m_connSort->setSourceModel(m_connModel);
    m_connSort->setSortRole(Qt::UserRole);
    m_connView = new QTreeView;
    m_connView->setModel(m_connSort);
    m_connView->setRootIsDecorated(false);
    m_connView->setUniformRowHeights(true);
    m_connView->setAllColumnsShowFocus(true);
    m_connView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connView->setSortingEnabled(true);
    m_connView->sortByColumn(ConnectionsModel::ColTimeStatus, Qt::AscendingOrder);
    m_connView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_connView->addAction(m_actCopyConn);
    auto *h = m_connView->header();
    h->resizeSection(ConnectionsModel::ColApplication, 190);
    h->resizeSection(ConnectionsModel::ColTarget, 260);
    h->resizeSection(ConnectionsModel::ColProto, 90);
    h->resizeSection(ConnectionsModel::ColTimeStatus, 110);
    h->resizeSection(ConnectionsModel::ColRuleProxy, 280);
    h->resizeSection(ConnectionsModel::ColSent, 90);
    connect(m_connView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        if (selectedRows().isEmpty())
            return;
        QMenu menu;
        menu.addAction(m_actDisconnect);
        menu.addAction(m_actEndProcess);
        menu.addSeparator();
        menu.addAction(m_actCreateRule);
        menu.addSeparator();
        menu.addAction(m_actCopyConn);
        menu.exec(m_connView->viewport()->mapToGlobal(pos));
    });
    m_tabs->addTab(m_connView, tr("Connections"));

    m_traffic = new TrafficWidget;
    m_tabs->addTab(m_traffic, tr("Traffic"));
    m_stats = new StatisticsWidget([this](const QString &path) { return m_connModel->appIcon(path); });
    m_tabs->addTab(m_stats, tr("Statistics"));
    splitter->addWidget(m_tabs);

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(10000);
    m_log->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_log->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_log, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu;
        menu.addAction(m_actLogCopy);
        menu.addAction(m_actLogSelectAll);
        menu.addAction(m_actLogClear);
        menu.addSeparator();
        QMenu *lvl = menu.addMenu(tr("L&og Level"));
        lvl->addActions(m_logLevelGroup->actions());
        QMenu *fs = menu.addMenu(tr("&Font Size"));
        fs->addActions(m_fontGroup->actions());
        menu.exec(m_log->viewport()->mapToGlobal(pos));
    });
    splitter->addWidget(m_log);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);
}

void MainWindow::createStatusBar()
{
    m_statusState = new QLabel;
    m_statusConns = new QLabel(tr("0 active connections"));
    m_statusRate = new QLabel;
    m_statusProfile = new QLabel;
    m_statusDriver = new QLabel;
    m_certButton = new QToolButton;
    m_certButton->setAutoRaise(true);
    m_certButton->setCursor(Qt::PointingHandCursor);
    m_certButton->setIcon(icons::certificate());
    m_certButton->setIconSize(QSize(14, 14));
    m_certButton->setFocusPolicy(Qt::NoFocus);
    m_certButton->hide();
    connect(m_certButton, &QToolButton::clicked, this, &MainWindow::showCertificateDetails);
    statusBar()->addWidget(m_statusState, 1);
    statusBar()->addPermanentWidget(m_statusDriver);
    statusBar()->addPermanentWidget(m_certButton);
    statusBar()->addPermanentWidget(m_statusConns);
    statusBar()->addPermanentWidget(m_statusRate);
    statusBar()->addPermanentWidget(m_statusProfile);
    connect(m_actStatusBar, &QAction::toggled, statusBar(), &QStatusBar::setVisible);
    updateDriverStatus();
}

void MainWindow::updateDriverStatus()
{
    if (!m_statusDriver)
        return;

    // Shows the certificate button for a backend's signatures. A generic lambda
    // serves both backends, whose cert lists have the same name/subject/issuer.
    auto applyCerts = [this](const auto &certs) {
        if (certs.isEmpty()) {
            m_certButton->hide();
        } else {
            m_certButton->setToolTip(certs.size() == 1
                                         ? tr("Signed by %1 — click for details").arg(certs.first().name)
                                         : tr("%1 signatures — click for details").arg(certs.size()));
            m_certButton->show();
        }
    };

    if (m_engine->backendKey() == QLatin1String("windivert")) {
        const auto info = WinDivertInterceptor::info();
        if (!info.available) {
            m_statusDriver->setText(tr("Backend: WinDivert (unavailable)"));
            m_statusDriver->setToolTip(QString());
            m_certButton->hide();
            return;
        }
        QStringList parts{"WinDivert"};
        if (!info.version.isEmpty())
            parts << "v" + info.version;
        parts << (info.running ? tr("running") : tr("not loaded"));
        m_statusDriver->setText(tr("Backend: %1").arg(parts.join(" · ")));
        m_statusDriver->setToolTip(info.running ? tr("WinDivert is capturing connections")
                                                : tr("WinDivert is not loaded"));
        applyCerts(info.certs);
        return;
    }

    const auto info = NetFilterInterceptor::installedDriverInfo();
    if (!info.installed) {
        m_statusDriver->setText(tr("Driver: not installed"));
        m_statusDriver->setToolTip(QString());
        m_certButton->hide();
        return;
    }
    const QString kind = info.demo ? tr("Demo") : (info.key.isEmpty() ? tr("Unknown") : tr("Unlimited"));
    QStringList parts{info.name};
    if (!info.version.isEmpty())
        parts << "v" + info.version;
    parts << kind;
    m_statusDriver->setText(tr("Driver: %1").arg(parts.join(" · ")));
    m_statusDriver->setToolTip(info.running ? tr("The network driver is running")
                                            : tr("The network driver is installed but not running"));
    applyCerts(info.certs);
}

void MainWindow::showCertificateDetails()
{
    const bool windivert = m_engine->backendKey() == QLatin1String("windivert");
    const QString name = windivert ? QStringLiteral("WinDivert")
                                    : NetFilterInterceptor::installedDriverInfo().name;

    // A generic lambda builds the tabbed dialog from either backend's cert list.
    auto show = [this, &name](const auto &certs) {
        if (certs.isEmpty()) {
            QMessageBox::information(this, tr("Driver Signature"),
                                     tr("The driver is not signed, or its signature could not be read."));
            return;
        }
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Driver Signature"));
        auto *lay = new QVBoxLayout(&dlg);
        lay->addWidget(new QLabel(tr("Signing certificates of %1:").arg(name)));
        auto *tabs = new QTabWidget;
        for (int i = 0; i < certs.size(); ++i) {
            const auto &c = certs[i];
            QString body = tr("Subject:") + "\n" + (c.subject.isEmpty() ? c.name : c.subject);
            if (!c.issuer.isEmpty())
                body += "\n\n" + tr("Issued by: %1").arg(c.issuer);
            auto *view = new QPlainTextEdit(body);
            view->setReadOnly(true);
            QString title = c.name.isEmpty() ? tr("Certificate %1").arg(i + 1) : c.name;
            if (title.size() > 28)
                title = title.left(27) + QChar(0x2026);
            tabs->addTab(view, title);
        }
        lay->addWidget(tabs, 1);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        lay->addWidget(bb);
        dlg.resize(480, 320);
        dlg.exec();
    };

    if (windivert)
        show(WinDivertInterceptor::info().certs);
    else
        show(NetFilterInterceptor::installedDriverInfo().certs);
}

void MainWindow::createTray()
{
    m_tray = new QSystemTrayIcon(icons::app(false), this);
    m_trayMenu = new QMenu(this);
    QAction *open = m_trayMenu->addAction(tr("&Open WinRoute"), this, &MainWindow::showAndActivate);
    QFont f = open->font();
    f.setBold(true);
    open->setFont(f);
    m_trayMenu->addSeparator();
    m_trayProfileMenu = m_trayMenu->addMenu(tr("Profiles"));
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_actExit);
    m_tray->setContextMenu(m_trayMenu);
    m_tray->setToolTip(tr("WinRoute"));
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) {
            if (isVisible() && !isMinimized())
                hide();
            else
                showAndActivate();
        }
    });
}

// ---- engine / window -----------------------------------------------------------

void MainWindow::startEngine()
{
    const QString backend = m_engine->backendKey() == QLatin1String("windivert")
                                ? QStringLiteral("WinDivert")
                                : QStringLiteral("NetFilter");
    // One report per launch: version, update preference and the active backend.
    switch (Updater::mode()) {
    case Updater::Mode::Automatic: UsageStats::reportStart(QStringLiteral("Automatic"), backend); break;
    case Updater::Mode::RequiredOnly: UsageStats::reportStart(QStringLiteral("Required only"), backend); break;
    case Updater::Mode::Ask: UsageStats::reportStart(QStringLiteral("Ask"), backend); break;
    case Updater::Mode::Never: UsageStats::reportStart(QStringLiteral("Never"), backend); break;
    }

    QString error;
    if (m_engine->start(&error)) {
        logNormal(tr("Network interception started (%1)").arg(m_engine->backendName()));
        m_statusState->setText(tr("Ready"));
        updateTrayIcon();
        probeLocalDns();
        UsageStats::report(QStringLiteral("Engine"), QStringLiteral("Started"), backend);
    } else {
        logError(tr("Cannot start network interception: ") + error);
        m_statusState->setText(tr("Not running: ") + error);
        UsageStats::report(QStringLiteral("Engine"), QStringLiteral("Failed"), backend);
    }
    updateDriverStatus();
    if (!m_startMinimized || !m_tray->isVisible())
        show();
    if (!m_engine->isRunning() && isVisible()) {
        auto *box = new QMessageBox(QMessageBox::Critical, tr("WinRoute"),
                                    tr("WinRoute cannot start network interception:\n\n") + error, QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    }
}

void MainWindow::showAndActivate()
{
    if (isMinimized())
        showNormal();
    else
        show();
    raise();
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QSettings s = util::appSettings();
    s.setValue("window/geometry", saveGeometry());
    s.setValue("window/state", saveState());
    if (!m_quitting && m_tray->isVisible()) {
        hide();
        e->ignore();
        return;
    }
    m_engine->stop();
    m_tray->hide();
    e->accept();
    QApplication::quit();
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange && isMinimized() && m_tray->isVisible())
        QTimer::singleShot(0, this, &QWidget::hide);
    QMainWindow::changeEvent(e);
}

void MainWindow::updateTitle()
{
    setWindowTitle(tr("WinRoute - %1").arg(m_profileName));
    m_statusProfile->setText(tr("Profile: ") + m_profileName);
    m_actHandleDirect->setChecked(m_profile.options.handleDirectConnections);
    m_actLoopDetection->setChecked(m_profile.options.loopDetection);
    m_actUdpProxy->setChecked(m_profile.options.udpMode == "mode_proxy");
    m_actLogDns->setChecked(m_profile.options.logDns);
    // Checking through a proxy needs at least one proxy in the profile.
    m_actCheckUpdatesProxy->setEnabled(!m_profile.proxies.isEmpty());
}

void MainWindow::updateTrayIcon()
{
    m_tray->setIcon(icons::app(m_engine->isRunning()));
}

// ---- profiles ------------------------------------------------------------------

QString MainWindow::profilesDir()
{
    const QString dir = QDir(util::dataDir()).filePath("Profiles");
    QDir().mkpath(dir);
    return dir;
}

QString MainWindow::profilePath(const QString &name) const
{
    return QDir(profilesDir()).filePath(name + ".ppx");
}

QStringList MainWindow::profileNames() const
{
    QStringList names;
    for (const QFileInfo &fi : QDir(profilesDir()).entryInfoList({"*.ppx"}, QDir::Files, QDir::Name))
        names << fi.completeBaseName();
    return names;
}

void MainWindow::initialProfile()
{
    QString name = util::appSettings().value("profile/current", "Default").toString();
    if (profileNames().isEmpty()) {
        // First start: take over the user's Proxifier profile if there is one.
        const QString appData = qEnvironmentVariable("APPDATA");
        const QStringList candidates{QDir(appData).filePath("Proxifier4/Profiles/Default.ppx"),
                                     QDir(appData).filePath("Proxifier/Profiles/Default.ppx")};
        bool imported = false;
        for (const QString &c : candidates) {
            Profile p;
            if (QFileInfo::exists(c) && p.load(c)) {
                p.save(profilePath("Default"));
                logNormal(tr("Imported Proxifier profile ") + QDir::toNativeSeparators(c));
                imported = true;
                break;
            }
        }
        if (!imported)
            Profile::createDefault().save(profilePath("Default"));
        name = "Default";
    }
    if (!loadProfile(name) && !loadProfile(profileNames().value(0))) {
        m_profileName = "Default";
        applyProfile(Profile::createDefault());
    }
}

bool MainWindow::loadProfile(const QString &name)
{
    if (name.isEmpty())
        return false;
    Profile p;
    QString error;
    if (!p.load(profilePath(name), &error)) {
        logError(tr("Cannot load profile %1: %2").arg(name, error));
        return false;
    }
    if (!error.isEmpty())
        logError(error);
    m_profileName = name;
    util::appSettings().setValue("profile/current", name);
    applyProfile(p);
    logNormal(tr("Profile %1 loaded").arg(name));
    return true;
}

void MainWindow::saveProfile()
{
    QString error;
    if (!m_profile.save(profilePath(m_profileName), &error))
        logError(tr("Cannot save profile %1: %2").arg(m_profileName, error));
}

void MainWindow::applyProfile(const Profile &p)
{
    m_profile = p;
    m_engine->setProfile(m_profile);
    saveProfile();
    updateTitle();
    rebuildProfileMenus();
}

void MainWindow::rebuildProfileMenus()
{
    for (QMenu *menu : {m_loadProfileMenu, m_trayProfileMenu}) {
        menu->clear();
        const QStringList names = profileNames();
        if (names.isEmpty())
            menu->addAction(tr("No profiles defined"))->setEnabled(false);
        for (const QString &n : names) {
            QAction *a = menu->addAction(n, this, [this, n] { loadProfile(n); });
            a->setCheckable(true);
            a->setChecked(n == m_profileName);
            a->setStatusTip(tr("Load this profile"));
        }
    }
}

static bool askProfileName(QWidget *parent, const QString &title, QString *name)
{
    bool ok = false;
    *name = QInputDialog::getText(parent, title,
                                  QCoreApplication::translate("MainWindow", "Profile Name:\n\nHint: WinRoute automatically loads the most recently used profile on start."),
                                  QLineEdit::Normal, *name, &ok)
                .trimmed();
    static const QRegularExpression bad(R"([\\/:*?"<>|])");
    if (ok && (name->isEmpty() || name->contains(bad))) {
        QMessageBox::warning(parent, title, QCoreApplication::translate("MainWindow", "The profile name is empty or contains characters not allowed in file names."));
        return false;
    }
    return ok;
}

void MainWindow::newProfile()
{
    QString name = tr("New Profile");
    if (!askProfileName(this, tr("New Profile"), &name))
        return;
    if (QFileInfo::exists(profilePath(name))
        && QMessageBox::question(this, tr("New Profile"), tr("Profile %1 already exists. Overwrite it?").arg(name)) != QMessageBox::Yes)
        return;
    m_profileName = name;
    util::appSettings().setValue("profile/current", name);
    applyProfile(Profile::createDefault());
    logNormal(tr("New profile %1 created").arg(name));
}

void MainWindow::saveProfileAs()
{
    QString name = m_profileName;
    if (!askProfileName(this, tr("Save Profile As"), &name) || name == m_profileName)
        return;
    if (QFileInfo::exists(profilePath(name))
        && QMessageBox::question(this, tr("Save Profile As"), tr("Profile %1 already exists. Overwrite it?").arg(name)) != QMessageBox::Yes)
        return;
    m_profileName = name;
    util::appSettings().setValue("profile/current", name);
    applyProfile(m_profile);
    logNormal(tr("Profile saved as %1").arg(name));
}

void MainWindow::importProfile()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Import Profile"), QString(), tr("Profile Files (*.ppx);;All Files (*.*)"));
    if (file.isEmpty())
        return;
    Profile p;
    QString error;
    if (!p.load(file, &error)) {
        QMessageBox::warning(this, tr("Import Profile"), tr("Cannot import %1:\n%2").arg(file, error));
        return;
    }
    if (!error.isEmpty())
        QMessageBox::warning(this, tr("Import Profile"), error);
    QString name = QFileInfo(file).completeBaseName();
    if (!askProfileName(this, tr("Import Profile"), &name))
        return;
    m_profileName = name;
    util::appSettings().setValue("profile/current", name);
    applyProfile(p);
    logNormal(tr("Profile %1 imported from %2").arg(name, QDir::toNativeSeparators(file)));
}

void MainWindow::exportProfile()
{
    const QString file = QFileDialog::getSaveFileName(this, tr("Export Profile"), m_profileName + ".ppx", tr("Profile Files (*.ppx)"));
    if (file.isEmpty())
        return;
    QString error;
    if (!m_profile.save(file, &error))
        QMessageBox::warning(this, tr("Export Profile"), error);
    else
        logNormal(tr("Profile exported to %1").arg(QDir::toNativeSeparators(file)));
}

void MainWindow::manageProfiles()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(profilesDir()));
}

// ---- profile settings ------------------------------------------------------------

void MainWindow::showProxyServers()
{
    ProxyServersDialog dlg(m_profile, this);
    connect(&dlg, &ProxyServersDialog::resetFailuresRequested, this, [this] {
        m_engine->resetProxyFailures();
        logNormal(tr("Fail status of all proxies was reset"));
    });
    if (dlg.exec() == QDialog::Accepted)
        applyProfile(dlg.profile());
}

void MainWindow::showRules()
{
    RulesDialog dlg(m_profile, this);
    if (dlg.exec() == QDialog::Accepted)
        applyProfile(dlg.profile());
}

void MainWindow::showNameResolution()
{
    NameResolutionDialog dlg(m_profile.options, this);
    if (dlg.exec() == QDialog::Accepted) {
        Profile p = m_profile;
        dlg.apply(&p.options);
        applyProfile(p);
        probeLocalDns();
    }
}

void MainWindow::showHttpProxies()
{
    HttpProxiesDialog dlg(m_profile.options.httpProxiesSupport, this);
    if (dlg.exec() == QDialog::Accepted) {
        Profile p = m_profile;
        p.options.httpProxiesSupport = dlg.enabled();
        applyProfile(p);
    }
}

void MainWindow::showServices()
{
    ServicesDialog dlg(m_profile.options, this);
    if (dlg.exec() == QDialog::Accepted) {
        Profile p = m_profile;
        dlg.apply(&p.options);
        applyProfile(p);
    }
}

void MainWindow::showEncryption()
{
    EncryptionDialog dlg(m_profile.options.encryption, this);
    if (dlg.exec() == QDialog::Accepted) {
        Profile p = m_profile;
        p.options.encryption = dlg.mode();
        applyProfile(p);
    }
}

void MainWindow::showProxyChecker()
{
    QList<QPair<QString, QList<ProxyServer>>> choices;
    for (const ProxyServer &p : m_profile.proxies)
        choices << qMakePair(QString("%1 [%2]").arg(p.displayName(), proxyTypeToString(p.type)), QList<ProxyServer>{p});
    for (const ProxyChain &c : m_profile.chains) {
        QList<ProxyServer> hops;
        for (const ChainMember &m : c.members)
            if (m.enabled)
                if (const ProxyServer *p = m_profile.proxyById(m.proxyId))
                    hops << *p;
        if (!hops.isEmpty())
            choices << qMakePair(tr("Chain %1").arg(c.name), c.type == ChainType::Simple ? hops : QList<ProxyServer>{hops.first()});
    }
    if (choices.isEmpty()) {
        QMessageBox::information(this, tr("Proxy Checker"), tr("Please add a proxy server first (Profile > Upstream Proxies)."));
        return;
    }
    ProxyCheckerDialog dlg(choices, this);
    dlg.exec();
}

void MainWindow::showProxyServerSettings()
{
    ProxyGatewayDialog dlg(ProxyGateway::loadConfig(), m_profile, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    const ProxyGateway::Config cfg = dlg.config();
    ProxyGateway::saveConfig(cfg);
    QString error;
    if (m_engine->applyGatewayConfig(cfg, &error))
        logNormal(tr("Proxy gateway settings saved."));
    else
        logError(tr("Proxy gateway: %1").arg(error));
}

void MainWindow::showAbout()
{
    // A fixed epigraph (Exodus 3:14), shown as-is in Hebrew; not translated.
    const QString motto = QString::fromUtf8(
        "<p align='center' dir='rtl' style='font-size:13pt'>אֶהְיֶה אֲשֶׁר אֶהְיֶה</p>");
    QMessageBox::about(this, tr("About WinRoute"),
                       motto
                           + tr("<h3>WinRoute %1</h3>"
                               "<p>Routes the network connections of Windows applications through SOCKS and HTTPS "
                               "proxy servers according to proxification rules.</p>"
                               "<p>Interception: %2</p>"
                               "<p>Qt %3</p>")
                                 // Always list both backends, not just the active one.
                                 .arg(QApplication::applicationVersion(),
                                      QStringLiteral("NetFilter SDK WFP driver / WinDivert"),
                                      QString::fromLatin1(qVersion()).section(QLatin1Char('.'), 0, 1)));
}

void MainWindow::checkForUpdatesViaProxy()
{
    if (m_profile.proxies.isEmpty())
        return; // the menu item is disabled in this case

    QStringList items;
    for (const ProxyServer &p : m_profile.proxies)
        items << p.displayName();

    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, tr("Check for Updates"),
                                                 tr("Check for and download updates through:"), items, 0, false, &ok);
    const int index = items.indexOf(chosen);
    if (!ok || index < 0)
        return;

    const ProxyServer &p = m_profile.proxies.at(index);
    // Qt has no SOCKS4 proxy; SOCKS4/SOCKS5 both use Socks5Proxy (a SOCKS4-only
    // server would fail, which the user sees as a failed download).
    const QNetworkProxy::ProxyType type =
        (p.type == ProxyType::Http || p.type == ProxyType::Https) ? QNetworkProxy::HttpProxy
                                                                  : QNetworkProxy::Socks5Proxy;
    m_updater->checkNow(QNetworkProxy(type, p.address, p.port, p.authEnabled ? p.username : QString(),
                                      p.authEnabled ? p.password : QString()));
}

// ---- connections ------------------------------------------------------------------

QList<int> MainWindow::selectedRows() const
{
    QList<int> rows;
    for (const QModelIndex &idx : m_connView->selectionModel()->selectedRows())
        rows << m_connSort->mapToSource(idx).row();
    return rows;
}

void MainWindow::disconnectSelected()
{
    for (int row : selectedRows())
        m_engine->closeConnection(m_connModel->idAt(row));
}

void MainWindow::endProcessSelected()
{
    QSet<quint32> pids;
    for (int row : selectedRows())
        pids.insert(m_connModel->pidAt(row));
    if (pids.isEmpty())
        return;
    QStringList names;
    for (quint32 pid : pids)
        names << tr("%1 (pid %2)").arg(procinfo::lookup(pid).name).arg(pid);
    if (QMessageBox::question(this, tr("End Process"), tr("Terminate the following process(es)?\n\n") + names.join('\n')) != QMessageBox::Yes)
        return;
    for (quint32 pid : pids) {
        QString err;
        if (!procinfo::terminate(pid, &err))
            logError(tr("Cannot terminate process %1: %2").arg(pid).arg(err));
    }
}

void MainWindow::createRuleForSelected()
{
    const QList<int> rows = selectedRows();
    if (rows.isEmpty())
        return;
    const QString app = m_connModel->data(m_connModel->index(rows.first(), ConnectionsModel::ColApplication), Qt::DisplayRole).toString();
    RulesDialog dlg(m_profile, this);
    dlg.show();
    dlg.addRuleFor(app.contains(' ') ? '"' + app + '"' : app);
    if (dlg.exec() == QDialog::Accepted)
        applyProfile(dlg.profile());
}

void MainWindow::copyConnections()
{
    QStringList lines;
    for (int row : selectedRows())
        lines << m_connModel->rowText(row);
    QApplication::clipboard()->setText(lines.join('\n'));
}

// ---- log --------------------------------------------------------------------------

void MainWindow::onLogEntry(int level, const QString &line)
{
    if (level > m_logLevel)
        return;
    QScrollBar *sb = m_log->verticalScrollBar();
    const bool atBottom = sb->value() == sb->maximum();
    if (level == int(LogLevel::Error))
        m_log->appendHtml(QString("<span style='color:#c0201a'>%1</span>").arg(line.toHtmlEscaped()));
    else if (level >= int(LogLevel::Verbose))
        m_log->appendHtml(QString("<span style='color:#606878'>%1</span>").arg(line.toHtmlEscaped()));
    else
        m_log->appendHtml(line.toHtmlEscaped());
    if (atBottom)
        sb->setValue(sb->maximum());
}

void MainWindow::setLogLevel(int level)
{
    m_logLevel = level;
    for (QAction *a : m_logLevelGroup->actions())
        a->setChecked(a->data().toInt() == level);
    util::appSettings().setValue("log/level", level);
}

void MainWindow::setLogFont(int size)
{
    QFont f("Consolas");
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(size);
    m_log->setFont(f);
    for (QAction *a : m_fontGroup->actions())
        a->setChecked(a->data().toInt() == size);
    util::appSettings().setValue("log/font", size);
}

// ---- traffic ---------------------------------------------------------------------

void MainWindow::onOpened(const ConnectionEvent &ev)
{
    m_connModel->opened(ev);
    m_stats->opened(ev);
    m_lastTotals.insert(ev.id, {0, 0});
}

void MainWindow::onTraffic(const QList<TrafficSample> &samples)
{
    for (const auto &s : samples) {
        auto &last = m_lastTotals[s.id];
        m_pendingOut += s.sent - last.first;
        m_pendingIn += s.received - last.second;
        last = {s.sent, s.received};
    }
    m_connModel->traffic(samples);
    m_stats->traffic(samples);
}

void MainWindow::onClosed(quint64 id, qint64 sent, qint64 received)
{
    const auto last = m_lastTotals.take(id);
    m_pendingOut += sent - last.first;
    m_pendingIn += received - last.second;
    m_connModel->closed(id, sent, received);
    m_stats->closed(id, sent, received);
}

void MainWindow::onSecondTick()
{
    const double in = double(m_pendingIn), out = double(m_pendingOut);
    m_pendingIn = m_pendingOut = 0;
    m_traffic->addSample(in, out);
    m_statusRate->setText(tr("Down %1, Up %2").arg(util::formatRate(in), util::formatRate(out)));
    if (m_tray->isVisible()) {
        updateTrayIcon();
        m_tray->setToolTip(tr("WinRoute - %1\n%2 active connections\nDown %3, Up %4")
                               .arg(m_profileName)
                               .arg(m_connModel->activeCount())
                               .arg(util::formatRate(in), util::formatRate(out)));
    }
}

void MainWindow::setRefreshRate(int ms)
{
    m_tickTimer.start(ms);
    for (QAction *a : m_refreshGroup->actions())
        a->setChecked(a->data().toInt() == ms);
    util::appSettings().setValue("view/refresh", ms);
}

void MainWindow::probeLocalDns()
{
    if (!m_profile.options.dnsAutoDetect || !m_engine->isRunning() || !m_dnsProbe)
        return;
    m_dnsProbe->resolve("www.msftconnecttest.com", [this](const QList<QHostAddress> &addrs, const QString &) {
        static int last = -1;
        const bool ok = !addrs.isEmpty();
        m_engine->setLocalDnsAvailable(ok);
        if (last != int(ok))
            logVerbose(ok ? tr("Local DNS is available, host names are resolved locally")
                          : tr("Local DNS is not available, host names are resolved through proxy"));
        last = int(ok);
    });
}

// ---- language --------------------------------------------------------------------

void MainWindow::changeLanguage(const QString &code)
{
    if (code == i18n::configured())
        return;
    i18n::setConfigured(code);
    if (QMessageBox::question(this, tr("Language"),
                              tr("The new language will be used after WinRoute restarts.\n\nRestart WinRoute now?"))
        != QMessageBox::Yes)
        return;
    QProcess::startDetached(QApplication::applicationFilePath(), {"--restart"});
    m_quitting = true;
    close();
}

// ---- autostart -------------------------------------------------------------------

// WinRoute needs administrator rights, so a scheduled task with highest
// privileges is used instead of the Run registry key (which would trigger UAC).
bool MainWindow::autostartEnabled()
{
    QProcess p;
    p.start("schtasks", {"/Query", "/TN", kTaskName});
    p.waitForFinished(5000);
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

void MainWindow::setAutostart(bool on)
{
    QProcess p;
    if (on) {
        const QString exe = QDir::toNativeSeparators(QApplication::applicationFilePath());
        p.start("schtasks", {"/Create", "/F", "/RL", "HIGHEST", "/SC", "ONLOGON", "/TN", kTaskName, "/TR",
                             QString("\"%1\" --minimized").arg(exe)});
    } else {
        p.start("schtasks", {"/Delete", "/F", "/TN", kTaskName});
    }
    p.waitForFinished(10000);
    if (p.exitCode() != 0) {
        logError(tr("Cannot change autostart: ") + QString::fromLocal8Bit(p.readAllStandardError()).trimmed());
        m_actAutostart->blockSignals(true);
        m_actAutostart->setChecked(!on);
        m_actAutostart->blockSignals(false);
    } else {
        logNormal(on ? tr("Autostart enabled") : tr("Autostart disabled"));
    }
}
