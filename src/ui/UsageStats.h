#pragma once

#include <QString>

// Anonymous usage statistics, sent to Matomo (WINROUTE_MATOMO_URL, site
// WINROUTE_MATOMO_SITE_ID) through its HTTP tracking API: which versions are
// running, which capture backend and driver are used, and how updating goes.
// Each install is a random ID made on first use; nothing about the user, their
// connections, targets, proxies or files is ever sent. On by default, can be
// turned off in Help > Send Anonymous Usage Statistics (which forgets the ID).
//
// Events (category / action / name):
//   App     / Start      / version          every start
//   App     / First run  / version          the first start of an install
//   App     / Updated    / "old -> new"      the first start after the version changed
//   Update  / Mode       / update mode       every start (Automatic/Required only/Ask/Never)
//   Update  / Available  / version           a newer version was found at start
//   Update  / Declined   / version           the user chose "Later"
//   Update  / Failed     / version           downloading or starting the update failed
//   Backend / Active     / NetFilter|WinDivert   every start, the selected backend
//   Backend / Switch     / NetFilter|WinDivert   the user switched backend
//   Engine  / Started    / NetFilter|WinDivert   interception started
//   Engine  / Failed     / NetFilter|WinDivert   interception could not start
//   Driver  / Install    / driver            a NetFilter driver was installed
//   Driver  / Switch      / driver            the active NetFilter driver was switched
//   Driver  / Uninstall  / NetFilter|WinDivert   a driver was uninstalled
namespace UsageStats {

// False when the build has no Matomo site to send to.
bool isAvailable();
bool isEnabled();
void setEnabled(bool enabled);

// At start: App/Start, Update/Mode, Backend/Active, and First run or Updated
// when they apply. updateMode and backend are the canonical English names.
void reportStart(const QString &updateMode, const QString &backend);
void report(const QString &category, const QString &action, const QString &name = QString());

} // namespace UsageStats
