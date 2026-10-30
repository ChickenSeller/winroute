#include "AdvancedDialogs.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

static QDialogButtonBox *okCancel(QDialog *dlg)
{
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(bb, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    return bb;
}

static QLabel *wrapped(const QString &text)
{
    auto *l = new QLabel(text);
    l->setWordWrap(true);
    return l;
}

HttpProxiesDialog::HttpProxiesDialog(bool enabled, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("HTTP Proxy Servers Support"));
    auto *layout = new QVBoxLayout(this);
    auto *box = new QGroupBox(tr("HTTP Proxy Servers"));
    auto *bl = new QVBoxLayout(box);
    m_enable = new QCheckBox(tr("Enable HTTP proxy servers support"));
    m_enable->setChecked(enabled);
    bl->addWidget(m_enable);
    bl->addWidget(wrapped(tr("When this option is enabled you can add HTTP proxy servers at the Upstream Proxies dialog.")));
    layout->addWidget(box);
    auto *warn = new QGroupBox(tr("WARNING"));
    auto *wl = new QVBoxLayout(warn);
    wl->addWidget(wrapped(tr("HTTP proxy server can only be used for HTTP connections (web browsers and etc). "
                          "WinRoute talks to HTTP proxies with CONNECT requests.")));
    wl->addWidget(wrapped(tr("Not to be confused with HTTPS connections and HTTPS proxy servers!")));
    layout->addWidget(warn);
    layout->addWidget(okCancel(this));
    setFixedWidth(420);
}

bool HttpProxiesDialog::enabled() const
{
    return m_enable->isChecked();
}

ServicesDialog::ServicesDialog(const ProfileOptions &o, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Services and other users"));
    auto *layout = new QVBoxLayout(this);
    auto *info = new QGroupBox(tr("Info"));
    auto *il = new QVBoxLayout(info);
    il->addWidget(wrapped(tr("Normally WinRoute processes connections for the current user only. "
                          "You can change this behavior if needed.")));
    il->addWidget(wrapped(tr("Note: these options only affect TCP and UDP. ICMP traffic, such as "
                          "ping and tracert, cannot be routed through a proxy regardless of this setting.")));
    layout->addWidget(info);
    auto *targets = new QGroupBox(tr("Additional WinRoute targets"));
    auto *tl = new QVBoxLayout(targets);
    m_otherUsers = new QCheckBox(tr("Applications run by other users on this computer"));
    m_otherUsers->setChecked(o.processOtherUsers);
    m_services = new QCheckBox(tr("Windows services and other system processes"));
    m_services->setChecked(o.processServices);
    tl->addWidget(m_otherUsers);
    tl->addWidget(m_services);
    layout->addWidget(targets);
    layout->addWidget(okCancel(this));
    setFixedWidth(420);
}

void ServicesDialog::apply(ProfileOptions *o) const
{
    o->processOtherUsers = m_otherUsers->isChecked();
    o->processServices = m_services->isChecked();
}

EncryptionDialog::EncryptionDialog(PasswordEncryption mode, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Profile Passwords Encryption"));
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(wrapped(tr("To provide additional security WinRoute can store proxy passwords in the profile file in "
                              "encrypted form.")));
    layout->addWidget(new QLabel(tr("Please choose one of the following modes:")));
    // QRadioButton text does not wrap, so each mode is a short radio plus a wrapped description.
    auto group = [&](const QString &title, const QString &desc) {
        auto *box = new QGroupBox(title);
        auto *bl = new QHBoxLayout(box);
        auto *rb = new QRadioButton;
        auto *label = wrapped(desc);
        label->setBuddy(rb);
        bl->addWidget(rb, 0, Qt::AlignTop);
        bl->addWidget(label, 1);
        layout->addWidget(box);
        return rb;
    };
    m_disabled = group(tr("Disabled"), tr("Passwords are saved in plain text. Proxy passwords can be used by everyone who has "
                                   "access to the profile file."));
    m_basic = group(tr("Basic"), tr("Encryption with a WinRoute static key. Proxy passwords can be used by everyone who opens "
                             "the profile with WinRoute."));
    m_user = group(tr("Current user account"),
                   tr("Encryption using current user login credentials. Only current user can access proxy passwords."));
    m_master = group(tr("Profile master password"), tr("Encryption with a profile master password (not supported)."));
    m_master->setEnabled(false);
    switch (mode) {
    case PasswordEncryption::Disabled: m_disabled->setChecked(true); break;
    case PasswordEncryption::Basic: m_basic->setChecked(true); break;
    case PasswordEncryption::User: m_user->setChecked(true); break;
    case PasswordEncryption::Master: m_disabled->setChecked(true); break;
    }
    layout->addWidget(okCancel(this));
    setFixedWidth(520);
}

PasswordEncryption EncryptionDialog::mode() const
{
    if (m_basic->isChecked())
        return PasswordEncryption::Basic;
    if (m_user->isChecked())
        return PasswordEncryption::User;
    return PasswordEncryption::Disabled;
}
