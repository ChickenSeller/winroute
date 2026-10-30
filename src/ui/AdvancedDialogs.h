#pragma once

#include "core/Profile.h"

#include <QDialog>

#include <memory>

namespace Ui {
class HttpProxiesDialog;
class ServicesDialog;
class EncryptionDialog;
}

// Profile > Advanced > HTTP Proxy Servers...  (layout in HttpProxiesDialog.ui)
class HttpProxiesDialog : public QDialog {
    Q_OBJECT
public:
    HttpProxiesDialog(bool enabled, QWidget *parent = nullptr);
    ~HttpProxiesDialog() override;
    bool enabled() const;

private:
    std::unique_ptr<Ui::HttpProxiesDialog> ui;
};

// Profile > Advanced > Services and Other Users...  (layout in ServicesDialog.ui)
class ServicesDialog : public QDialog {
    Q_OBJECT
public:
    ServicesDialog(const ProfileOptions &o, QWidget *parent = nullptr);
    ~ServicesDialog() override;
    void apply(ProfileOptions *o) const;

private:
    std::unique_ptr<Ui::ServicesDialog> ui;
};

// Profile > Advanced > Passwords Encryption...  (layout in EncryptionDialog.ui)
class EncryptionDialog : public QDialog {
    Q_OBJECT
public:
    EncryptionDialog(PasswordEncryption mode, QWidget *parent = nullptr);
    ~EncryptionDialog() override;
    PasswordEncryption mode() const;

private:
    std::unique_ptr<Ui::EncryptionDialog> ui;
};
