#pragma once

#include "core/Profile.h"

#include <QDialog>

class QCheckBox;
class QPlainTextEdit;
class QRadioButton;

// "Name Resolution" settings (resolve through proxy, exclusion list).
class NameResolutionDialog : public QDialog {
    Q_OBJECT
public:
    NameResolutionDialog(const ProfileOptions &options, QWidget *parent = nullptr);
    void apply(ProfileOptions *options) const;

private:
    void update();

    QCheckBox *m_auto;
    QCheckBox *m_viaProxy;
    QCheckBox *m_localFirst;
    QRadioButton *m_exclude;
    QRadioButton *m_only;
    QPlainTextEdit *m_list;
};
