#include "DriverDialog.h"
#include "core/Engine.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

DriverDialog::DriverDialog(Engine *engine, QWidget *parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(tr("Network Driver"));
    setMinimumWidth(460);
    m_drivers = NetFilterInterceptor::availableDrivers();

    auto *layout = new QVBoxLayout(this);
    auto *intro = new QLabel(tr("WinRoute captures connections with a NetFilter SDK driver. "
                                "Choose which driver to use, or uninstall it."),
                             this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *form = new QFormLayout;
    m_combo = new QComboBox(this);
    for (const auto &d : m_drivers)
        m_combo->addItem(d.name, d.key);
    form->addRow(tr("Driver:"), m_combo);
    layout->addLayout(form);

    m_note = new QLabel(this);
    m_note->setWordWrap(true);
    m_note->setMinimumHeight(48);
    layout->addWidget(m_note);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    auto *buttons = new QDialogButtonBox(this);
    m_apply = buttons->addButton(tr("Use This Driver"), QDialogButtonBox::AcceptRole);
    m_uninstall = buttons->addButton(tr("Uninstall Driver"), QDialogButtonBox::DestructiveRole);
    buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    connect(m_combo, &QComboBox::currentIndexChanged, this, &DriverDialog::onSelectionChanged);
    connect(m_apply, &QPushButton::clicked, this, &DriverDialog::apply);
    connect(m_uninstall, &QPushButton::clicked, this, &DriverDialog::uninstall);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (m_drivers.isEmpty()) {
        m_combo->setEnabled(false);
        m_apply->setEnabled(false);
    }
    refresh();
}

void DriverDialog::refresh()
{
    const QString current = NetFilterInterceptor::installedDriverKey();
    const bool installed = NetFilterInterceptor::driverInstalled();
    if (installed) {
        const QString name = current.isEmpty() ? tr("an unrecognized driver")
                                               : NetFilterInterceptor::driverDisplayName(current);
        m_status->setText(NetFilterInterceptor::driverRunning()
                              ? tr("Installed: %1 (running)").arg(name)
                              : tr("Installed: %1 (stopped)").arg(name));
        if (!current.isEmpty()) {
            const int i = m_combo->findData(current);
            if (i >= 0)
                m_combo->setCurrentIndex(i);
        }
    } else {
        m_status->setText(tr("No driver is installed."));
    }
    m_uninstall->setEnabled(installed);
    onSelectionChanged();
}

void DriverDialog::onSelectionChanged()
{
    const QString key = m_combo->currentData().toString();
    if (key == "demo")
        m_note->setText(tr("The evaluation driver from the NetFilter SDK. It stops capturing "
                           "connections after a limited number of them, until Windows restarts."));
    else
        m_note->setText(tr("A release build of the driver, without a connection limit."));

    const QString current = NetFilterInterceptor::installedDriverKey();
    const bool isCurrent = !current.isEmpty() && current == key && NetFilterInterceptor::driverRunning();
    m_apply->setText(NetFilterInterceptor::driverInstalled() ? tr("Switch to This Driver") : tr("Install This Driver"));
    m_apply->setEnabled(!m_drivers.isEmpty() && !isCurrent);
}

bool DriverDialog::restartEngine(QString *error)
{
    return m_engine->start(error);
}

void DriverDialog::apply()
{
    const int i = m_combo->currentIndex();
    if (i < 0 || i >= m_drivers.size())
        return;
    const auto d = m_drivers[i];

    setEnabled(false);
    const bool wasRunning = m_engine->isRunning();
    m_engine->stop();   // release the driver handle before swapping it

    QString error;
    bool ok = NetFilterInterceptor::installDriver(&error, true, d.path);
    if (ok && wasRunning)
        ok = restartEngine(&error);

    setEnabled(true);
    if (ok) {
        emit log(tr("Network driver switched to %1").arg(d.name));
        refresh();
    } else {
        emit log(tr("Could not switch the network driver: %1").arg(error));
        QMessageBox::warning(this, tr("Network Driver"),
                             tr("Could not switch to %1:\n%2").arg(d.name, error));
        refresh();
    }
}

void DriverDialog::uninstall()
{
    if (QMessageBox::question(this, tr("Network Driver"),
                              tr("Uninstall the network driver? WinRoute cannot capture connections without it."))
        != QMessageBox::Yes)
        return;

    setEnabled(false);
    m_engine->stop();
    QString error;
    const bool ok = NetFilterInterceptor::uninstallDriver(&error);
    setEnabled(true);
    if (ok) {
        emit log(tr("Network driver uninstalled"));
    } else {
        emit log(tr("Could not uninstall the network driver: %1").arg(error));
        QMessageBox::warning(this, tr("Network Driver"), tr("Could not uninstall the driver:\n%1").arg(error));
    }
    refresh();
}
