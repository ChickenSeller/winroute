#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVersionNumber>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QSaveFile;

// Keeps WinRoute up to date from the homepage's update.json (which the web
// server reads live from the GitLab releases, see deploy/nginx/), as Glimpse does:
//
//   {"latest":   {"version", "url", "sha256", "size", "notes"} or null,
//    "required": the same, or null}
//
// "required" is the version every older WinRoute must move to (set on the
// web server). What happens depends on Help > Automatic Updates: install the
// latest automatically, install a required version automatically, ask about
// the latest, or never check.
//
// Installing runs the downloaded setup silently once WinRoute has quit, then
// starts the new version: resources/update.ps1 does that. The setup keeps the
// installed driver and the tasks chosen before.
class Updater : public QObject {
    Q_OBJECT
public:
    enum class Mode {
        Automatic,    // install the latest version without asking
        RequiredOnly, // install a required version without asking, nothing else
        Ask,          // offer the latest version
        Never,
    };

    explicit Updater(QObject *parent = nullptr);
    ~Updater() override;

    static Mode mode();
    static void setMode(Mode mode);

    // Checks once, as the settings say.
    void checkAtStartup();
    // Checks now on the user's request, whatever the settings: offers a newer
    // version, or says that there is none or that the check failed.
    void checkNow();

signals:
    // For the tray: an automatic update started or failed.
    void notify(const QString &title, const QString &message);
    // The update has started and waits for WinRoute to exit.
    void quitRequested();

private:
    struct Release {
        QVersionNumber version;
        QUrl url;
        QByteArray sha256;
        qint64 size = 0;
        QString notes;
    };

    void requestManifest(bool interactive);
    void onManifest(QNetworkReply *reply, bool interactive);
    static bool parse(const QJsonObject &object, const QUrl &base, Release *release);
    void ask(const Release &release);
    void download(const Release &release, bool interactive);
    void onDownloaded(const Release &release, QNetworkReply *reply, bool interactive);
    bool install(const QString &setupPath, QString *error);

    QNetworkAccessManager *m_network;
    QSaveFile *m_file = nullptr;
    QPointer<QProgressDialog> m_progress;
};
