#pragma once

#include <QHostAddress>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace util {

// Application settings, kept in WinRoute.ini next to the executable (a portable
// INI, not the registry). WinRoute runs elevated, so it can write there even
// under Program Files. Returned by value (C++17 elision); use like QSettings().
QSettings appSettings();

// One-time move of settings from the old registry location
// (HKCU\Software\WinRoute\WinRoute) into WinRoute.ini, so an upgrade from a
// version that used the registry keeps the user's preferences. Does nothing if
// the INI already exists or the registry holds no settings. Call once at start,
// before any appSettings() access.
void migrateLegacySettings();

// Case-insensitive glob match supporting '*' and '?'.
bool wildcardMatch(QStringView pattern, QStringView text);

// Splits a Proxifier style list ("a; b; \"c d\"") into trimmed, unquoted items.
QStringList splitList(const QString &list);

// Joins an application list back into Proxifier's form: items separated by "; ",
// with any name that contains a space wrapped in double quotes (Proxifier rejects
// an unquoted space with "Unexpected blank space found").
QString joinAppList(const QString &list);

// Expands %VAR% occurrences using the process environment. %ComputerName% is always known.
QString expandEnv(const QString &s);

QString computerName();

// %APPDATA%\WinRoute: profiles and the log file.
QString dataDir();

QString formatBytes(qint64 bytes);        // "512 bytes", "2.29 KB", "1.50 MB"
QString formatRate(double bytesPerSec);   // "12.3 KB/sec"
QString formatDuration(qint64 seconds);   // "00:05", "01:02:03"

QString formatEndpoint(const QHostAddress &addr, quint16 port);   // "1.2.3.4:80", "[::1]:80"

// Converts IPv4-mapped IPv6 addresses to plain IPv4.
QHostAddress normalize(const QHostAddress &addr);

} // namespace util
