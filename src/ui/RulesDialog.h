#pragma once

#include "core/Profile.h"

#include <QDialog>

#include <memory>

namespace Ui {
class RulesDialog;
}

class QTreeWidget;
class QTreeWidgetItem;

// "Proxification Rules" list: ordered, checkable, Default rule pinned last.
class RulesDialog : public QDialog {
    Q_OBJECT
public:
    RulesDialog(const Profile &profile, QWidget *parent = nullptr);
    ~RulesDialog() override;
    Profile profile() const { return m_profile; }

    // Opens the rule editor for a new rule pre-filled with the given application.
    void addRuleFor(const QString &application);

private:
    void rebuild(int select);
    int current() const;
    void add();
    void clone();
    void edit();
    void remove();
    void move(int delta);

    std::unique_ptr<Ui::RulesDialog> ui;
    Profile m_profile;
    QTreeWidget *m_list;
};
