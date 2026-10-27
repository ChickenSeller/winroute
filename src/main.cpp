#include "core/Engine.h"
#include "core/Logger.h"
#include "core/NetFilterInterceptor.h"
#include "core/WinDivertInterceptor.h"
#include "core/ProcessInfo.h"
#include "ui/Language.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QSettings>
#include <QLocalSocket>
#include <QMessageBox>
#include <QThread>

#include <winsock2.h>
#include <windows.h>

static const char *kInstanceKey = "WinRoute-single-instance";
// Checked by the installer (AppMutex) to detect a running WinRoute.
static const wchar_t *kAppMutex = L"WinRoute.AppMutex";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("WinRoute");
    QApplication::setApplicationVersion(WINROUTE_VERSION);
    // QSettings needs an organization name (stored under HKCU\Software\WinRoute\WinRoute).
    QApplication::setOrganizationName("WinRoute");
    QApplication::setQuitOnLastWindowClosed(false);
    i18n::install(app);

    // --install-driver / --uninstall-driver are run by the installer; with --quiet
    // errors are only reported through the exit code (0 = success, 1 = failure).
    const QStringList args = QApplication::arguments();
    // The installer passes --install-driver=<key> to install the chosen driver;
    // a bare --install-driver (or an unknown key) installs the default.
    bool installDriver = false;
    QString installKey;
    for (const QString &a : args) {
        if (a == "--install-driver")
            installDriver = true;
        else if (a.startsWith("--install-driver=")) {
            installDriver = true;
            installKey = a.mid(QStringLiteral("--install-driver=").size());
        }
    }
    // The installer selects the capture backend here. WinDivert installs its own
    // driver on first use, so it only needs the backend preference recorded.
    if (installDriver && installKey == QLatin1String("windivert")) {
        QSettings().setValue("backend/active", "windivert");
        return 0;
    }
    if (installDriver || args.contains("--uninstall-driver")) {
        QString error;
        QString source;
        for (const auto &d : NetFilterInterceptor::availableDrivers())
            if (d.key == installKey)
                source = d.path;
        bool ok;
        if (installDriver) {
            QSettings().setValue("backend/active", "netfilter");
            ok = NetFilterInterceptor::installDriver(&error, true, source);
        } else {
            // Remove both backends' drivers; WinDivert installs itself on first
            // use, so a missing service is not an error.
            ok = NetFilterInterceptor::uninstallDriver(&error);
            QString wdError;
            WinDivertInterceptor::uninstall(&wdError);
        }
        if (!ok && !args.contains("--quiet"))
            QMessageBox::critical(nullptr, "WinRoute",
                                  installDriver
                                      ? QCoreApplication::translate("main", "Cannot install the network driver: %1").arg(error)
                                      : QCoreApplication::translate("main", "Cannot uninstall the network driver: %1").arg(error));
        return ok ? 0 : 1;
    }

    // After a restart (e.g. language change) give the previous instance time to exit.
    if (args.contains("--restart")) {
        for (int i = 0; i < 100; ++i) {
            QLocalSocket probe;
            probe.connectToServer(kInstanceKey);
            if (!probe.waitForConnected(100))
                break;
            probe.abort();
            QThread::msleep(100);
        }
    }

    // Only one instance may own the driver; a second start just shows the first window.
    {
        QLocalSocket probe;
        probe.connectToServer(kInstanceKey);
        if (probe.waitForConnected(300)) {
            probe.write("show");
            probe.waitForBytesWritten(300);
            return 0;
        }
    }
    HANDLE appMutex = CreateMutexW(nullptr, FALSE, kAppMutex);
    QLocalServer instanceServer;
    QLocalServer::removeServer(kInstanceKey);
    instanceServer.listen(kInstanceKey);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    Engine engine;
    MainWindow window(&engine);
    window.setStartMinimized(args.contains("--minimized"));
    QObject::connect(&instanceServer, &QLocalServer::newConnection, &window, [&] {
        while (QLocalSocket *s = instanceServer.nextPendingConnection()) {
            QObject::connect(s, &QLocalSocket::disconnected, s, &QObject::deleteLater);
            window.showAndActivate();
        }
    });
    window.startEngine();
    const int rc = app.exec();
    engine.stop();
    WSACleanup();
    if (appMutex)
        CloseHandle(appMutex);
    return rc;
}
