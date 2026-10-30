#include "DriverDialog.h"
#include "ui_DriverDialog.h"

#include "UsageStats.h"
#include "core/Engine.h"
#include "core/WinDivertInterceptor.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

DriverDialog::DriverDialog(Engine *engine, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::DriverDialog>()), m_engine(engine)
{
    ui->setupUi(this);
    setMinimumWidth(460);
    m_drivers = NetFilterInterceptor::availableDrivers();

    m_backendCombo = ui->backendCombo;
    m_combo = ui->driverCombo;
    m_driverLabel = ui->driverLabel;
    m_note = ui->noteLabel;
    m_status = ui->statusLabel;

    m_backendCombo->addItem(tr("NetFilter SDK driver"), "netfilter");
    m_backendCombo->addItem(WinDivertInterceptor::available() ? tr("WinDivert")
                                                              : tr("WinDivert (not available)"),
                            "windivert");
    for (const auto &d : m_drivers)
        m_combo->addItem(d.name, d.key);

    // Custom-role buttons are added here; the Close button is in the .ui.
    m_apply = ui->buttonBox->addButton(tr("Use This Driver"), QDialogButtonBox::AcceptRole);
    m_uninstall = ui->buttonBox->addButton(tr("Uninstall Driver"), QDialogButtonBox::DestructiveRole);

    const QString backend = m_engine->backendKey();
    const int bi = m_backendCombo->findData(backend);
    if (bi >= 0)
        m_backendCombo->setCurrentIndex(bi);

    connect(m_backendCombo, &QComboBox::currentIndexChanged, this, &DriverDialog::refresh);
    connect(m_combo, &QComboBox::currentIndexChanged, this, &DriverDialog::onSelectionChanged);
    connect(m_apply, &QPushButton::clicked, this, &DriverDialog::apply);
    connect(m_uninstall, &QPushButton::clicked, this, &DriverDialog::uninstall);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refresh();
}

DriverDialog::~DriverDialog() = default;

QString DriverDialog::selectedBackend() const
{
    return m_backendCombo->currentData().toString();
}

void DriverDialog::refresh()
{
    const bool netfilter = selectedBackend() == QLatin1String("netfilter");
    m_driverLabel->setVisible(netfilter);
    m_combo->setVisible(netfilter);

    if (netfilter) {
        if (!m_drivers.isEmpty()) {
            const QString current = NetFilterInterceptor::installedDriverKey();
            if (!current.isEmpty()) {
                const int i = m_combo->findData(current);
                if (i >= 0)
                    m_combo->setCurrentIndex(i);
            }
        }
        const bool installed = NetFilterInterceptor::driverInstalled();
        if (installed) {
            const QString current = NetFilterInterceptor::installedDriverKey();
            const QString name = current.isEmpty() ? tr("an unrecognized driver")
                                                   : NetFilterInterceptor::driverDisplayName(current);
            m_status->setText(NetFilterInterceptor::driverRunning()
                                  ? tr("Installed: %1 (running)").arg(name)
                                  : tr("Installed: %1 (stopped)").arg(name));
        } else {
            m_status->setText(tr("No NetFilter driver is installed."));
        }
        m_uninstall->setText(tr("Uninstall Driver"));
        m_uninstall->setEnabled(installed);
    } else {
        if (!WinDivertInterceptor::available()) {
            m_note->setText(tr("WinDivert.dll or its driver is missing next to WinRoute.exe, so this "
                               "backend cannot be used."));
            m_status->setText(QString());
            m_apply->setEnabled(false);
            m_uninstall->setEnabled(false);
            return;
        }
        m_note->setText(tr("WinDivert captures TCP and UDP at the packet level. Host names come from observed "
                           "DNS responses, or from WinRoute's fake DNS when \"Resolve hostnames through proxy\" "
                           "is on."));
        const auto info = WinDivertInterceptor::info();
        QStringList parts;
        if (!info.version.isEmpty())
            parts << "v" + info.version;
        parts << (info.running ? tr("running") : tr("not loaded"));
        m_status->setText(tr("WinDivert: %1").arg(parts.join(" · ")));
        m_uninstall->setText(tr("Uninstall WinDivert"));
        m_uninstall->setEnabled(WinDivertInterceptor::serviceRunning());
    }
    onSelectionChanged();
}

void DriverDialog::onSelectionChanged()
{
    const bool netfilter = selectedBackend() == QLatin1String("netfilter");
    const bool isCurrentBackend = selectedBackend() == m_engine->backendKey();

    if (netfilter) {
        const QString key = m_combo->currentData().toString();
        if (key == "demo")
            m_note->setText(tr("The evaluation driver from the NetFilter SDK. It stops capturing "
                               "connections after a limited number of them, until Windows restarts."));
        else
            m_note->setText(tr("A release build of the driver, without a connection limit."));

        const QString current = NetFilterInterceptor::installedDriverKey();
        const bool isCurrent = isCurrentBackend && !current.isEmpty() && current == key
                               && NetFilterInterceptor::driverRunning();
        m_apply->setText(NetFilterInterceptor::driverInstalled() ? tr("Switch to This Driver")
                                                                 : tr("Install This Driver"));
        m_apply->setEnabled(!m_drivers.isEmpty() && !isCurrent);
    } else {
        m_apply->setText(tr("Use WinDivert"));
        m_apply->setEnabled(WinDivertInterceptor::available()
                            && !(isCurrentBackend && m_engine->isRunning()));
    }
}

bool DriverDialog::restartEngine(QString *error)
{
    return m_engine->start(error);
}

void DriverDialog::apply()
{
    setEnabled(false);
    const bool wasRunning = m_engine->isRunning();
    const QString backend = selectedBackend();
    const bool backendChanged = backend != m_engine->backendKey();
    QString error;
    bool ok = true;
    QString done;
    QStringList driverEvent;   // {action, name} for a NetFilter driver operation, reported on success

    if (backend == QLatin1String("netfilter")) {
        const int i = m_combo->currentIndex();
        if (i < 0 || i >= m_drivers.size()) {
            setEnabled(true);
            return;
        }
        const auto d = m_drivers[i];
        const bool hadDriver = NetFilterInterceptor::driverInstalled();
        m_engine->stop();   // release any handle before swapping
        ok = m_engine->setBackend("netfilter", &error);
        if (ok)
            ok = NetFilterInterceptor::installDriver(&error, true, d.path);
        driverEvent = {hadDriver ? QStringLiteral("Switch") : QStringLiteral("Install"), d.key};
        done = tr("Network driver switched to %1").arg(d.name);
    } else {
        m_engine->stop();
        ok = m_engine->setBackend("windivert", &error);
        done = tr("Capture backend switched to WinDivert");
    }

    if (ok && wasRunning)
        ok = restartEngine(&error);

    setEnabled(true);
    if (ok) {
        const QString backendName = backend == QLatin1String("windivert") ? QStringLiteral("WinDivert")
                                                                          : QStringLiteral("NetFilter");
        if (backendChanged)
            UsageStats::report(QStringLiteral("Backend"), QStringLiteral("Switch"), backendName);
        if (!driverEvent.isEmpty())
            UsageStats::report(QStringLiteral("Driver"), driverEvent.value(0), driverEvent.value(1));
        emit log(done);
    } else {
        emit log(tr("Could not switch the capture backend: %1").arg(error));
        QMessageBox::warning(this, tr("Network Driver"), tr("Could not apply the change:\n%1").arg(error));
    }
    refresh();
}

void DriverDialog::uninstall()
{
    const bool netfilter = selectedBackend() == QLatin1String("netfilter");
    if (QMessageBox::question(this, tr("Network Driver"),
                              netfilter
                                  ? tr("Uninstall the NetFilter driver? WinRoute cannot capture connections "
                                       "with this backend without it.")
                                  : tr("Uninstall the WinDivert driver service?"))
        != QMessageBox::Yes)
        return;

    setEnabled(false);
    m_engine->stop();
    QString error;
    const bool ok = netfilter ? NetFilterInterceptor::uninstallDriver(&error)
                              : WinDivertInterceptor::uninstall(&error);
    setEnabled(true);
    if (ok) {
        UsageStats::report(QStringLiteral("Driver"), QStringLiteral("Uninstall"),
                           netfilter ? QStringLiteral("NetFilter") : QStringLiteral("WinDivert"));
        emit log(netfilter ? tr("Network driver uninstalled") : tr("WinDivert service uninstalled"));
    } else
        QMessageBox::warning(this, tr("Network Driver"), tr("Could not uninstall:\n%1").arg(error));
    refresh();
}
