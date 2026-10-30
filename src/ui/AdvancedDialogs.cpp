#include "AdvancedDialogs.h"
#include "ui_HttpProxiesDialog.h"
#include "ui_ServicesDialog.h"
#include "ui_EncryptionDialog.h"

HttpProxiesDialog::HttpProxiesDialog(bool enabled, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::HttpProxiesDialog>())
{
    ui->setupUi(this);
    ui->enableCheck->setChecked(enabled);
    setFixedWidth(420);
}

HttpProxiesDialog::~HttpProxiesDialog() = default;

bool HttpProxiesDialog::enabled() const
{
    return ui->enableCheck->isChecked();
}

ServicesDialog::ServicesDialog(const ProfileOptions &o, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::ServicesDialog>())
{
    ui->setupUi(this);
    ui->otherUsersCheck->setChecked(o.processOtherUsers);
    ui->servicesCheck->setChecked(o.processServices);
    setFixedWidth(420);
}

ServicesDialog::~ServicesDialog() = default;

void ServicesDialog::apply(ProfileOptions *o) const
{
    o->processOtherUsers = ui->otherUsersCheck->isChecked();
    o->processServices = ui->servicesCheck->isChecked();
}

EncryptionDialog::EncryptionDialog(PasswordEncryption mode, QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::EncryptionDialog>())
{
    ui->setupUi(this);
    // The master-password mode is not supported; its radio stays disabled (.ui).
    switch (mode) {
    case PasswordEncryption::Disabled: ui->disabledRadio->setChecked(true); break;
    case PasswordEncryption::Basic: ui->basicRadio->setChecked(true); break;
    case PasswordEncryption::User: ui->userRadio->setChecked(true); break;
    case PasswordEncryption::Master: ui->disabledRadio->setChecked(true); break;
    }
    setFixedWidth(520);
}

EncryptionDialog::~EncryptionDialog() = default;

PasswordEncryption EncryptionDialog::mode() const
{
    if (ui->basicRadio->isChecked())
        return PasswordEncryption::Basic;
    if (ui->userRadio->isChecked())
        return PasswordEncryption::User;
    return PasswordEncryption::Disabled;
}
