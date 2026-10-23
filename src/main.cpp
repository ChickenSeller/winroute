#include "core/Engine.h"
#include "core/Logger.h"
#include "core/NetFilterInterceptor.h"
#include "core/ProcessInfo.h"
#include "ui/Language.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QThread>

#include <winsock2.h>
#include <windows.h>

static const char *kInstanceKey = "WinRoute-single-instance";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("WinRoute");
    QApplication::setApplicationVersion("1.0.0");
    // QSettings needs an organization name (stored under HKCU\Software\WinRoute\WinRoute).
    QApplication::setOrganizationName("WinRoute");
    QApplication::setQuitOnLastWindowClosed(false);
    i18n::install(app);

    const QStringList args = QApplication::arguments();
    if (args.contains("--uninstall-driver")) {
        QString error;
        const bool ok = NetFilterInterceptor::uninstallDriver(&error);
        if (!ok)
            QMessageBox::critical(nullptr, "WinRoute",
                                  QCoreApplication::translate("main", "Cannot uninstall the network driver: %1").arg(error));
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
    return rc;
}
