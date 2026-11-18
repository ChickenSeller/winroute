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
    // `profile` provides the options and the proxies and chains to choose from.
    NameResolutionDialog(const Profile &profile, QWidget *parent = nullptr);
    ~NameResolutionDialog() override;

    void apply(ProfileOptions *options) const;

private:
    void update();

    std::unique_ptr<Ui::NameResolutionDialog> ui;
    Profile m_profile;            // the proxies and chains to choose from
    int m_directId = 0;           // chosen resolver for direct connections
    bool m_directActive = false;  // the list shows the proxies (it applies)
};
