#pragma once

#include "core/Profile.h"

#include <QDialog>

class QCheckBox;
class QRadioButton;

// Profile > Advanced > HTTP Proxy Servers...
class HttpProxiesDialog : public QDialog {
    Q_OBJECT
public:
    HttpProxiesDialog(bool enabled, QWidget *parent = nullptr);
    bool enabled() const;

private:
    QCheckBox *m_enable;
};

// Profile > Advanced > Services and Other Users...
class ServicesDialog : public QDialog {
    Q_OBJECT
public:
    ServicesDialog(const ProfileOptions &o, QWidget *parent = nullptr);
    void apply(ProfileOptions *o) const;

private:
    QCheckBox *m_otherUsers;
    QCheckBox *m_services;
};

// Profile > Advanced > Passwords Encryption...
class EncryptionDialog : public QDialog {
    Q_OBJECT
public:
    EncryptionDialog(PasswordEncryption mode, QWidget *parent = nullptr);
    PasswordEncryption mode() const;

private:
    QRadioButton *m_disabled;
    QRadioButton *m_basic;
    QRadioButton *m_user;
    QRadioButton *m_master;
};
