#include "HttpParse.h"
#include "Capture.h"

#include <QList>

QString HttpMessage::header(const QString &name) const
{
    for (const HttpHeader &h : headers)
        if (h.name.compare(name, Qt::CaseInsensitive) == 0)
            return h.value;
    return QString();
}

namespace {

// One direction's bytes, concatenated, with a way to find the timestamp of any
// offset (the segment that offset falls in).
struct Stream {
    QByteArray data;
    QList<qint64> segEnd;    // cumulative end offset of each segment
    QList<qint64> segTs;     // timestamp of each segment

    qint64 tsAt(qint64 offset) const
    {
        for (int i = 0; i < segEnd.size(); ++i)
            if (offset < segEnd[i])
                return segTs[i];
        return segTs.isEmpty() ? 0 : segTs.last();
    }
};

Stream collect(const CaptureEntry &entry, CaptureDir dir)
{
    Stream s;
    for (const CaptureSegment &seg : entry.segs) {
        if (seg.dir != dir)
            continue;
        s.data.append(seg.data);
        s.segEnd.append(s.data.size());
        s.segTs.append(seg.tsMs);
    }
    return s;
}

const char *kMethods[] = {"GET ", "POST ", "PUT ", "HEAD ", "DELETE ",
                          "OPTIONS ", "PATCH ", "TRACE ", "CONNECT "};

bool startsWithMethod(const QByteArray &b)
{
    for (const char *m : kMethods)
        if (b.startsWith(m))
            return true;
    return false;
}

// Splits "Name: value" header lines from the block before the blank line.
void parseHeaders(const QByteArray &block, HttpMessage *msg)
{
    const QList<QByteArray> lines = block.split('\n');
    for (int i = 1; i < lines.size(); ++i) {   // line 0 is the start line
        QByteArray line = lines[i];
        if (line.endsWith('\r'))
            line.chop(1);
        if (line.isEmpty())
            continue;
        const int colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        HttpHeader h;
        h.name = QString::fromLatin1(line.left(colon)).trimmed();
        h.value = QString::fromLatin1(line.mid(colon + 1)).trimmed();
        msg->headers.append(h);
    }
}

// Decodes a chunked body starting at `pos`; returns the decoded bytes and sets
// `end` past the final chunk (0\r\n\r\n), or complete=false when truncated.
QByteArray decodeChunked(const QByteArray &buf, int pos, int *end, bool *complete)
{
    QByteArray out;
    *complete = false;
    while (pos < buf.size()) {
        const int nl = buf.indexOf('\n', pos);
        if (nl < 0)
            break;
        QByteArray sizeLine = buf.mid(pos, nl - pos).trimmed();
        const int semi = sizeLine.indexOf(';');
        if (semi >= 0)
            sizeLine = sizeLine.left(semi);
        bool ok = false;
        const int chunkSize = sizeLine.toInt(&ok, 16);
        if (!ok)
            break;
        pos = nl + 1;
        if (chunkSize == 0) {
            *complete = true;
            // Skip optional trailers up to the final blank line.
            const int trailerEnd = buf.indexOf("\r\n", pos);
            *end = (trailerEnd >= 0) ? trailerEnd + 2 : buf.size();
            break;
        }
        if (pos + chunkSize > buf.size())
            break;
        out.append(buf.mid(pos, chunkSize));
        pos += chunkSize;
        if (buf.mid(pos, 2) == "\r\n")
            pos += 2;
    }
    if (!*complete)
        *end = buf.size();
    return out;
}

// Parses one message at `pos`. Returns the number of bytes consumed, or -1 when
// there is not even a complete header block yet.
int parseOne(const QByteArray &buf, int pos, bool isRequest, HttpMessage *msg)
{
    const int headerEnd = buf.indexOf("\r\n\r\n", pos);
    if (headerEnd < 0)
        return -1;
    const QByteArray headBlock = buf.mid(pos, headerEnd - pos);
    const int firstNl = headBlock.indexOf('\n');
    QByteArray startLine = firstNl < 0 ? headBlock : headBlock.left(firstNl);
    if (startLine.endsWith('\r'))
        startLine.chop(1);

    msg->isRequest = isRequest;
    if (isRequest) {
        const QList<QByteArray> parts = startLine.split(' ');
        if (parts.size() < 3)
            return -1;
        msg->method = QString::fromLatin1(parts[0]);
        msg->uri = QString::fromLatin1(parts[1]);
        msg->version = QString::fromLatin1(parts[2]);
    } else {
        const int sp1 = startLine.indexOf(' ');
        if (sp1 < 0)
            return -1;
        msg->version = QString::fromLatin1(startLine.left(sp1));
        const int sp2 = startLine.indexOf(' ', sp1 + 1);
        const QByteArray code = sp2 < 0 ? startLine.mid(sp1 + 1) : startLine.mid(sp1 + 1, sp2 - sp1 - 1);
        msg->statusCode = code.toInt();
        msg->reason = sp2 < 0 ? QString() : QString::fromLatin1(startLine.mid(sp2 + 1));
    }
    parseHeaders(headBlock, msg);
    msg->valid = true;

    int bodyStart = headerEnd + 4;
    const QString te = msg->header("Transfer-Encoding");
    const QString cl = msg->header("Content-Length");
    if (te.contains("chunked", Qt::CaseInsensitive)) {
        int end = 0;
        msg->body = decodeChunked(buf, bodyStart, &end, &msg->complete);
        return end - pos;
    }
    if (!cl.isEmpty()) {
        const int len = cl.toInt();
        const int have = qMin(len, buf.size() - bodyStart);
        msg->body = buf.mid(bodyStart, have);
        msg->complete = (have == len);
        return (bodyStart + len) - pos;
    }
    // No length given: a request has no body; a response runs to end of stream.
    if (isRequest) {
        msg->complete = true;
        return bodyStart - pos;
    }
    msg->body = buf.mid(bodyStart);
    msg->complete = true;   // treat connection-close framing as complete
    return buf.size() - pos;
}

QList<HttpMessage> parseStream(const QByteArray &buf, bool isRequest, QList<int> *offsets)
{
    QList<HttpMessage> out;
    int pos = 0;
    while (pos < buf.size()) {
        HttpMessage msg;
        const int consumed = parseOne(buf, pos, isRequest, &msg);
        if (consumed <= 0) {
            if (consumed == 0)   // avoid an infinite loop on a zero-length parse
                break;
            if (msg.valid)       // a header-only message with no complete body yet
                { offsets->append(pos); out.append(msg); }
            break;
        }
        offsets->append(pos);
        out.append(msg);
        pos += consumed;
    }
    return out;
}

} // namespace

bool looksLikeHttpRequest(const QByteArray &bytes)
{
    return startsWithMethod(bytes);
}

QList<HttpExchange> parseHttp(const CaptureEntry &entry)
{
    const Stream up = collect(entry, CaptureDir::Up);
    const Stream down = collect(entry, CaptureDir::Down);
    if (!startsWithMethod(up.data))
        return {};

    QList<int> reqOff, respOff;
    const QList<HttpMessage> reqs = parseStream(up.data, true, &reqOff);
    const QList<HttpMessage> resps = parseStream(down.data, false, &respOff);
    if (reqs.isEmpty())
        return {};

    QList<HttpExchange> out;
    const int n = qMax(reqs.size(), resps.size());
    for (int i = 0; i < n; ++i) {
        HttpExchange ex;
        if (i < reqs.size()) {
            ex.request = reqs[i];
            ex.reqTsMs = up.tsAt(reqOff[i]);
        }
        if (i < resps.size()) {
            ex.response = resps[i];
            ex.respTsMs = down.tsAt(respOff[i]);
        }
        out.append(ex);
    }
    return out;
}
