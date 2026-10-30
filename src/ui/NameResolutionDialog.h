#pragma once

#include "core/Profile.h"

#include <QDialog>

#include <memory>

namespace Ui {
class NameResolutionDialog;
}

// "Name Resolution" settings (resolve through proxy, exclusion list). The layout
// lives in NameResolutionDialog.ui.
class NameResolutionDialog : public QDialog {
    Q_OBJECT
public:
    NameResolutionDialog(const ProfileOptions &options, QWidget *parent = nullptr);
    ~NameResolutionDialog() override;

    void apply(ProfileOptions *options) const;

private:
    void update();

    std::unique_ptr<Ui::NameResolutionDialog> ui;
};
