#include "Updater.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QWidget>

#include <algorithm>

namespace {

const char *kModeKey = "updates/mode";

QString updateDirectory()
{
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("winroute-update");
    QDir().mkpath(path);
    return path;
}

} // namespace

Updater::Updater(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

Updater::~Updater() = default;

Updater::Mode Updater::mode()
{
    const int value = QSettings().value(kModeKey, int(Mode::Ask)).toInt();
    return value >= 0 && value <= int(Mode::Never) ? Mode(value) : Mode::Ask;
}

void Updater::setMode(Mode mode)
{
    QSettings().setValue(kModeKey, int(mode));
}

void Updater::checkAtStartup()
{
    if (mode() != Mode::Never)
        requestManifest(false);
}

void Updater::checkNow()
{
    requestManifest(true);
}

void Updater::requestManifest(bool interactive)
{
    QNetworkRequest request{QUrl(QStringLiteral(WINROUTE_UPDATE_URL))};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(30000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, interactive] { onManifest(reply, interactive); });
}

bool Updater::parse(const QJsonObject &object, const QUrl &base, Release *release)
{
    release->version = QVersionNumber::fromString(object.value("version").toString());
    release->url = base.resolved(QUrl(object.value("url").toString()));
    release->sha256 = object.value("sha256").toString().toLatin1().toLower();
    release->size = object.value("size").toInteger();
    release->notes = object.value("notes").toString();
    return !release->version.isNull() && release->url.isValid() && !release->sha256.isEmpty();
}

void Updater::onManifest(QNetworkReply *reply, bool interactive)
{
    reply->deleteLater();
    // At start, being offline or a page without update information is not
    // worth a word; asked for, it is.
    if (reply->error() != QNetworkReply::NoError) {
        if (interactive)
            QMessageBox::warning(nullptr, tr("WinRoute Update"),
                                 tr("Could not check for updates:\n%1").arg(reply->errorString()));
        return;
    }
    const QJsonObject manifest = QJsonDocument::fromJson(reply->readAll()).object();
    const QVersionNumber current = QVersionNumber::fromString(QApplication::applicationVersion());

    Release latest;
    Release required;
    const bool haveLatest = parse(manifest.value("latest").toObject(), reply->url(), &latest) && latest.version > current;
    const bool haveRequired = parse(manifest.value("required").toObject(), reply->url(), &required)
                              && required.version > current;

    if (interactive) {
        if (haveLatest)
            ask(latest);
        else
            QMessageBox::information(nullptr, tr("WinRoute Update"),
                                     tr("WinRoute %1 is the latest version.").arg(QApplication::applicationVersion()));
        return;
    }

    switch (mode()) {
    case Mode::Automatic:
        if (haveLatest)
            download(latest, false);
        break;
    case Mode::RequiredOnly:
        if (haveRequired)
            download(required, false);
        break;
    case Mode::Ask:
        if (haveLatest)
            ask(latest);
        break;
    case Mode::Never:
        break;
    }
}

void Updater::ask(const Release &release)
{
    QMessageBox box;
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("WinRoute Update"));
    box.setText(tr("WinRoute %1 is available; you have %2.\nUpdate now? WinRoute restarts when it is done.")
                    .arg(release.version.toString(), QApplication::applicationVersion()));
    box.setDetailedText(release.notes);
    QPushButton *update = box.addButton(tr("Update"), QMessageBox::AcceptRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.setDefaultButton(update);
    box.setWindowFlag(Qt::WindowStaysOnTopHint);
    box.exec();
    if (box.clickedButton() == update)
        download(release, true);
}

void Updater::download(const Release &release, bool interactive)
{
    if (m_file)
        return; // one at a time

    const QString path = QDir(updateDirectory()).filePath(release.url.fileName());
    m_file = new QSaveFile(path, this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        delete m_file;
        m_file = nullptr;
        return;
    }

    QNetworkRequest request(release.url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(60000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] { m_file->write(reply->readAll()); });
    connect(reply, &QNetworkReply::finished, this, [this, release, reply, interactive] {
        onDownloaded(release, reply, interactive);
    });

    if (interactive) {
        m_progress = new QProgressDialog(tr("Downloading WinRoute %1...").arg(release.version.toString()), tr("Cancel"), 0, 0);
        m_progress->setWindowTitle(tr("WinRoute Update"));
        m_progress->setAttribute(Qt::WA_DeleteOnClose);
        m_progress->setMinimumDuration(0);
        connect(reply, &QNetworkReply::downloadProgress, m_progress, [this](qint64 received, qint64 total) {
            if (total > 0) {
                m_progress->setMaximum(int(total / 1024));
                m_progress->setValue(int(received / 1024));
            }
        });
        connect(m_progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
        m_progress->show();
    } else {
        emit notify(tr("WinRoute Update"),
                    tr("Downloading WinRoute %1; WinRoute restarts once it is installed.").arg(release.version.toString()));
    }
}

void Updater::onDownloaded(const Release &release, QNetworkReply *reply, bool interactive)
{
    reply->deleteLater();
    m_file->write(reply->readAll());
    if (m_progress)
        m_progress->close();

    QString error;
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        m_file->cancelWriting();
    } else if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        m_file->cancelWriting();
    } else if (!m_file->commit()) {
        error = m_file->errorString();
    } else {
        // Only the setup published with the release is run.
        QFile check(m_file->fileName());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!check.open(QIODevice::ReadOnly) || !hash.addData(&check) || hash.result().toHex() != release.sha256)
            error = tr("The download is damaged (checksum mismatch).");
    }
    const QString setupPath = m_file->fileName();
    delete m_file;
    m_file = nullptr;

    if (!error.isEmpty()) {
        QFile::remove(setupPath);
        const QString message = tr("Could not download WinRoute %1:\n%2").arg(release.version.toString(), error);
        if (interactive)
            QMessageBox::warning(nullptr, tr("WinRoute Update"), message);
        else
            emit notify(tr("WinRoute Update"), message);
        return;
    }
    if (reply->error() == QNetworkReply::OperationCanceledError)
        return;

    if (!install(setupPath, &error)) {
        if (interactive)
            QMessageBox::warning(nullptr, tr("WinRoute Update"), error);
        else
            emit notify(tr("WinRoute Update"), error);
    }
}

bool Updater::install(const QString &setupPath, QString *error)
{
    // The script is copied out of the resources: PowerShell needs a file.
    const QString script = QDir(updateDirectory()).filePath("update.ps1");
    QFile::remove(script);
    if (!QFile::copy(":/update.ps1", script)) {
        *error = tr("Could not prepare the update.");
        return false;
    }
    QFile::setPermissions(script, QFile::ReadOwner | QFile::WriteOwner);

    QStringList arguments = {
        "-NoProfile", "-ExecutionPolicy", "Bypass", "-WindowStyle", "Hidden",
        "-File", QDir::toNativeSeparators(script),
        "-Setup", QDir::toNativeSeparators(setupPath),
        "-ProcessId", QString::number(QCoreApplication::applicationPid()),
        "-Exe", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()),
    };
    // Back in the tray if WinRoute is there now (no window open).
    const QWidgetList windows = QApplication::topLevelWidgets();
    if (std::none_of(windows.begin(), windows.end(), [](QWidget *w) { return w->isWindow() && w->isVisible(); }))
        arguments << "-Minimized";
    // By its full path: PATH does not always include PowerShell's folder.
    const QString systemRoot = qEnvironmentVariable("SystemRoot", "C:\\Windows");
    QString powershell = QDir(systemRoot).filePath("System32/WindowsPowerShell/v1.0/powershell.exe");
    if (!QFileInfo::exists(powershell))
        powershell = "powershell.exe"; // let Windows look for it after all
    QProcess process;
    process.setProgram(QDir::toNativeSeparators(powershell));
    process.setArguments(arguments);
    if (!process.startDetached()) {
        *error = tr("Could not start the update.") + '\n' + process.errorString();
        return false;
    }
    // The script waits for WinRoute to exit, runs the setup and starts it again.
    emit quitRequested();
    return true;
}
