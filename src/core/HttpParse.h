#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

struct CaptureEntry;

struct HttpHeader {
    QString name;
    QString value;
};

struct HttpMessage {
    bool valid = false;
    bool complete = false;     // the body was fully received
    bool isRequest = false;
    // Request line
    QString method;
    QString uri;
    QString version;           // "HTTP/1.1"
    // Response line
    int statusCode = 0;
    QString reason;
    QList<HttpHeader> headers;
    QByteArray body;

    QString header(const QString &name) const;   // case-insensitive, first match
};

struct HttpExchange {
    HttpMessage request;
    HttpMessage response;
    qint64 reqTsMs = 0;
    qint64 respTsMs = 0;
};

// The HTTP/1.x exchanges in a capture entry, or an empty list when the stream
// is not HTTP. Requests come from the client->server bytes, responses from the
// server->client bytes, paired in order.
QList<HttpExchange> parseHttp(const CaptureEntry &entry);

// Whether the bytes begin with an HTTP/1.x request line ("GET / HTTP/1.1"...).
bool looksLikeHttpRequest(const QByteArray &bytes);
