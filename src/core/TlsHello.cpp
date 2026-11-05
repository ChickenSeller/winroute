#include "TlsHello.h"

namespace tlshello {

namespace {

constexpr int kRecordHeader = 5;
constexpr unsigned char kHandshake = 0x16;
constexpr unsigned char kClientHello = 0x01;
constexpr int kMaxHello = 64 * 1024;   // larger is not a ClientHello we handle

// Bounds-checked reader over the ClientHello body.
struct Reader {
    const unsigned char *p;
    int n;
    int pos = 0;
    bool ok = true;

    bool need(int k)
    {
        if (!ok || k < 0 || pos + k > n)
            ok = false;
        return ok;
    }
    int u8()
    {
        if (!need(1))
            return 0;
        return p[pos++];
    }
    int u16()
    {
        if (!need(2))
            return 0;
        const int v = (p[pos] << 8) | p[pos + 1];
        pos += 2;
        return v;
    }
    void skip(int k)
    {
        if (need(k))
            pos += k;
    }
};

} // namespace

Result serverName(const QByteArray &data, QString *serverName)
{
    serverName->clear();
    const auto *d = reinterpret_cast<const unsigned char *>(data.constData());
    const int size = int(data.size());

    // Join the fragments of the handshake records until the ClientHello is whole.
    QByteArray hs;
    int pos = 0;
    while (true) {
        if (size - pos < kRecordHeader)
            return (pos == 0 && size > 0 && d[0] != kHandshake) ? Result::NotTls : Result::NeedMore;
        if (d[pos] != kHandshake || d[pos + 1] != 0x03)
            return Result::NotTls;
        const int len = (d[pos + 3] << 8) | d[pos + 4];
        if (len == 0 || len > 16384 + 2048)
            return Result::NotTls;
        if (size - pos - kRecordHeader < len)
            return Result::NeedMore;
        hs.append(data.constData() + pos + kRecordHeader, len);
        pos += kRecordHeader + len;
        if (hs.size() >= 4) {
            const auto *h = reinterpret_cast<const unsigned char *>(hs.constData());
            if (h[0] != kClientHello)
                return Result::NotTls;
            const int bodyLen = (h[1] << 16) | (h[2] << 8) | h[3];
            if (bodyLen > kMaxHello)
                return Result::NotTls;
            if (hs.size() >= 4 + bodyLen) {
                hs = hs.mid(4, bodyLen);
                break;
            }
        }
    }

    Reader r{reinterpret_cast<const unsigned char *>(hs.constData()), int(hs.size())};
    r.skip(2);           // client_version
    r.skip(32);          // random
    r.skip(r.u8());      // session_id
    r.skip(r.u16());     // cipher_suites
    r.skip(r.u8());      // compression_methods
    if (!r.ok)
        return Result::NotTls;
    if (r.pos == r.n)
        return Result::Done;   // no extensions, so no SNI
    const int extEnd = r.pos + 2 + r.u16();
    if (!r.ok || extEnd > r.n)
        return Result::NotTls;
    while (r.ok && r.pos + 4 <= extEnd) {
        const int type = r.u16();
        const int len = r.u16();
        if (type != 0) {   // not server_name
            r.skip(len);
            continue;
        }
        const int end = r.pos + len;
        const int listEnd = r.pos + 2 + r.u16();
        if (!r.ok || listEnd > end)
            return Result::NotTls;
        while (r.ok && r.pos + 3 <= listEnd) {
            const int nameType = r.u8();
            const int nameLen = r.u16();
            if (!r.need(nameLen))
                return Result::NotTls;
            if (nameType == 0) {   // host_name
                *serverName = QString::fromLatin1(reinterpret_cast<const char *>(r.p + r.pos), nameLen).toLower();
                return Result::Done;
            }
            r.skip(nameLen);
        }
        return Result::Done;
    }
    return r.ok ? Result::Done : Result::NotTls;
}

} // namespace tlshello
