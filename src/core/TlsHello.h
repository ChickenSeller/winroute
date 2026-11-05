#pragma once

#include <QByteArray>
#include <QString>

// Reads the server name (SNI) from the start of a TLS connection: the name the
// client asks for, and so the name the certificate must be valid for. Used by
// HTTPS decryption to mint the leaf certificate for the right host, since an
// IP address shared by several sites (a CDN, or www. and down. of one site)
// does not say which one the client means.
namespace tlshello {

enum class Result {
    NeedMore,   // the ClientHello is not complete yet
    NotTls,     // not a TLS ClientHello
    Done,       // parsed; *serverName is the SNI host, or empty when none is sent
};

// `data` is what the client has sent so far (not consumed). The ClientHello
// may span several TLS records; they are joined.
Result serverName(const QByteArray &data, QString *serverName);

} // namespace tlshello
