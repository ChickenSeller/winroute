#pragma once

#include <QList>
#include <QString>

// Reads a Windows file's version and Authenticode signatures. Shared by the
// driver backends (NetFilter and WinDivert) for the status bar and the driver
// dialog. Windows-only (CryptoAPI + version.dll).
namespace codesign {

// One Authenticode signer of a file.
struct Cert {
    QString name;       // signer common name, for the tab title
    QString subject;    // full signer subject (multi-line)
    QString issuer;     // issuer common name
};

// File version as "a.b.c.d", or {} if the file has none.
QString fileVersion(const QString &path);

// Every Authenticode signature of the file (primary first, then nested ones),
// or empty if it is unsigned or the signature cannot be read.
QList<Cert> fileSigners(const QString &path);

} // namespace codesign
