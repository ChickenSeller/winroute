#include "Capture.h"
#include "HttpParse.h"
#include "RuleEngine.h"
#include "Utils.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

Capture::Capture(QObject *parent) : QObject(parent) {}

void Capture::setEnabled(bool on)
{
    m_enabled.store(on, std::memory_order_relaxed);
}

bool Capture::isEnabled() const
{
    return m_enabled.load(std::memory_order_relaxed);
}

void Capture::setDecrypt(bool on)
{
    m_decrypt.store(on, std::memory_order_relaxed);
}

bool Capture::decrypt() const
{
    return m_decrypt.load(std::memory_order_relaxed);
}

void Capture::setLimits(qint64 perConn, qint64 total)
{
    QMutexLocker lock(&m_mutex);
    m_perConn = perConn;
    m_total = total;
    evictLocked();
}

void Capture::setFilter(const CaptureFilter &filter)
{
    QMutexLocker lock(&m_mutex);
    m_filter = filter;
}

CaptureFilter Capture::filter() const
{
    QMutexLocker lock(&m_mutex);
    return m_filter;
}

bool Capture::shouldCapture(const QString &appName, const QString &appPath, const QString &host,
                            const QHostAddress &ip, quint16 port) const
{
    CaptureFilter f;
    {
        QMutexLocker lock(&m_mutex);
        f = m_filter;
    }
    if (f.mode == CaptureFilter::All)
        return true;

    // A rule matches when it has at least one field and every non-empty field
    // matches; the connection matches the list when any enabled rule matches.
    bool matched = false;
    for (const CaptureRule &r : f.rules) {
        if (!r.enabled)
            continue;
        bool any = false, ok = true;
        if (!r.applications.trimmed().isEmpty()) {
            any = true;
            if (!RuleEngine::matchAppList(util::splitList(r.applications), appPath, appName))
                ok = false;
        }
        if (ok && !r.targets.trimmed().isEmpty()) {
            any = true;
            if (!RuleEngine::matchHostList(util::splitList(r.targets), host, ip))
                ok = false;
        }
        if (ok && !r.ports.trimmed().isEmpty()) {
            any = true;
            if (!RuleEngine::matchPortList(r.ports, port))
                ok = false;
        }
        if (any && ok) {
            matched = true;
            break;
        }
    }
    return f.mode == CaptureFilter::Only ? matched : !matched;
}

void Capture::note(const CaptureMeta &meta)
{
    {
        QMutexLocker lock(&m_mutex);
        if (!m_enabled || m_entries.contains(meta.id))
            return;
        CaptureEntry e;
        e.meta = meta;
        e.open = true;
        m_entries.insert(meta.id, e);
        m_order.append(meta.id);
    }
    emit changed();
}

void Capture::append(quint64 id, CaptureDir dir, const QByteArray &bytes, qint64 tsMs, bool decrypted)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_entries.find(id);
    if (it == m_entries.end() || bytes.isEmpty())
        return;
    CaptureEntry &e = *it;
    if (decrypted)
        e.decrypted = true;

    // Guess the protocol from the first bytes we see.
    if (e.proto.isEmpty()) {
        if (bytes.size() >= 2 && quint8(bytes[0]) == 0x16 && quint8(bytes[1]) == 0x03)
            e.proto = QStringLiteral("TLS");
        else if (dir == CaptureDir::Up && looksLikeHttpRequest(bytes))
            e.proto = QStringLiteral("HTTP");
        else
            e.proto = QStringLiteral("TCP");
    }

    qint64 room = m_perConn > 0 ? m_perConn - (e.up + e.down) : bytes.size();
    if (room <= 0) {
        e.truncated = true;
        return;
    }
    const QByteArray stored = bytes.size() <= room ? bytes : bytes.left(int(room));
    if (stored.size() < bytes.size())
        e.truncated = true;

    e.segs.append(CaptureSegment{dir, tsMs, stored});
    if (dir == CaptureDir::Up)
        e.up += stored.size();
    else
        e.down += stored.size();
    m_bytes += stored.size();
    evictLocked();
}

void Capture::finish(quint64 id, qint64 tsMs)
{
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_entries.find(id);
        if (it == m_entries.end())
            return;
        it->open = false;
        it->endMs = tsMs;
    }
    emit changed();
}

void Capture::evictLocked()
{
    while (m_total > 0 && m_bytes > m_total && m_order.size() > 1) {
        const quint64 oldest = m_order.takeFirst();
        auto it = m_entries.find(oldest);
        if (it != m_entries.end()) {
            m_bytes -= (it->up + it->down);
            m_entries.erase(it);
        }
    }
}

QList<CaptureSummary> Capture::summaries() const
{
    QMutexLocker lock(&m_mutex);
    QList<CaptureSummary> out;
    out.reserve(m_order.size());
    for (quint64 id : m_order) {
        auto it = m_entries.constFind(id);
        if (it == m_entries.constEnd())
            continue;
        const CaptureEntry &e = *it;
        CaptureSummary s;
        s.meta = e.meta;
        s.up = e.up;
        s.down = e.down;
        s.endMs = e.endMs;
        s.open = e.open;
        s.truncated = e.truncated;
        s.decrypted = e.decrypted;
        s.proto = e.proto;
        out.append(s);
    }
    return out;
}

CaptureEntry Capture::entry(quint64 id) const
{
    QMutexLocker lock(&m_mutex);
    return m_entries.value(id);
}

bool Capture::hasEntry(quint64 id) const
{
    QMutexLocker lock(&m_mutex);
    return m_entries.contains(id);
}

int Capture::count() const
{
    QMutexLocker lock(&m_mutex);
    return m_entries.size();
}

void Capture::clear()
{
    {
        QMutexLocker lock(&m_mutex);
        m_entries.clear();
        m_order.clear();
        m_bytes = 0;
    }
    emit changed();
}

// ---- PCAPNG export ---------------------------------------------------------
// Each connection becomes a synthesized TCP stream: a three-way handshake, then
// one segment per captured run. Endpoints are the real application and target
// addresses/ports, so Wireshark reassembles the stream and (for decrypted TLS
// or plain HTTP) shows the payload.

namespace {

void putLE16(QByteArray &b, quint16 v) { b.append(char(v & 0xff)); b.append(char(v >> 8)); }
void putLE32(QByteArray &b, quint32 v)
{
    for (int i = 0; i < 4; ++i)
        b.append(char((v >> (8 * i)) & 0xff));
}
void putLE64(QByteArray &b, quint64 v)
{
    for (int i = 0; i < 8; ++i)
        b.append(char((v >> (8 * i)) & 0xff));
}
void putBE16(QByteArray &b, quint16 v) { b.append(char(v >> 8)); b.append(char(v & 0xff)); }
void putBE32(QByteArray &b, quint32 v)
{
    b.append(char((v >> 24) & 0xff));
    b.append(char((v >> 16) & 0xff));
    b.append(char((v >> 8) & 0xff));
    b.append(char(v & 0xff));
}

quint16 ipChecksum(const QByteArray &hdr)
{
    quint32 sum = 0;
    for (int i = 0; i + 1 < hdr.size(); i += 2)
        sum += (quint8(hdr[i]) << 8) | quint8(hdr[i + 1]);
    if (hdr.size() & 1)
        sum += quint8(hdr[hdr.size() - 1]) << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return quint16(~sum);
}

// Whether an address is a real, specific host (not null and not the "any"
// 0.0.0.0 / :: that the interceptor reports at connect time).
bool isSpecific(const QHostAddress &a)
{
    if (a.isNull())
        return false;
    if (a.protocol() == QAbstractSocket::IPv4Protocol)
        return a.toIPv4Address() != 0;
    if (a.protocol() == QAbstractSocket::IPv6Protocol) {
        const Q_IPV6ADDR v = a.toIPv6Address();
        for (int i = 0; i < 16; ++i)
            if (v[i])
                return true;
    }
    return false;
}

QByteArray ipBytesOf(const QHostAddress &a, bool v6)
{
    QByteArray out;
    if (v6) {
        const Q_IPV6ADDR v = a.toIPv6Address();
        out = QByteArray(reinterpret_cast<const char *>(v.c), 16);
    } else {
        const quint32 x = a.toIPv4Address();
        out.resize(4);
        out[0] = char((x >> 24) & 0xff); out[1] = char((x >> 16) & 0xff);
        out[2] = char((x >> 8) & 0xff);  out[3] = char(x & 0xff);
    }
    return out;
}

// A deterministic placeholder address for when the real one is unknown (the
// application's local address at connect time, or a fake-DNS destination).
// Clients land in 10.x.x.x / fd0a::, servers in 198.18.x.x (the benchmarking
// range) / fd18:: — clearly not real routable hosts. The Name Resolution Block
// maps them back to the host and application names, so Wireshark shows those.
QByteArray synthIp(const QString &seed, bool v6, bool server)
{
    const quint32 h = qHash(seed, 0x9e3779b9u);
    const quint8 b2 = (h >> 16) & 0xff, b3 = (h >> 8) & 0xff;
    const quint8 b4 = (h & 0xff) ? (h & 0xff) : 1;
    if (v6) {
        QByteArray out(16, 0);
        out[0] = char(0xfd);
        out[1] = char(server ? 0x18 : 0x0a);
        out[13] = char(b2); out[14] = char(b3); out[15] = char(b4);
        return out;
    }
    QByteArray out(4, 0);
    out[0] = char(server ? 198 : 10);
    out[1] = char(server ? 18 : b2);
    out[2] = char(b3);
    out[3] = char(b4);
    return out;
}

// One Name Resolution Block record: IP -> name, so captures read with the host
// and application names rather than the placeholder addresses.
void writeNrbRecord(QByteArray &body, bool v6, const QByteArray &ip, const QString &name)
{
    QByteArray val = ip;
    val += name.toUtf8();
    val += '\0';
    putLE16(body, v6 ? 2 : 1);   // nrb_record_ipv6 / nrb_record_ipv4
    putLE16(body, quint16(val.size()));
    body.append(val);
    while (body.size() % 4)
        body.append(char(0));
}

// One synthesized Ethernet+IP+TCP frame.
QByteArray makeFrame(bool v6, bool toServer, const QByteArray &cAddr, const QByteArray &sAddr,
                     quint16 cPort, quint16 sPort, quint32 seq, quint32 ack, quint8 flags,
                     const QByteArray &payload, quint16 ipId)
{
    const QByteArray &src = toServer ? cAddr : sAddr;
    const QByteArray &dst = toServer ? sAddr : cAddr;
    const quint16 sp = toServer ? cPort : sPort;
    const quint16 dp = toServer ? sPort : cPort;

    QByteArray tcp;
    putBE16(tcp, sp);
    putBE16(tcp, dp);
    putBE32(tcp, seq);
    putBE32(tcp, ack);
    tcp.append(char(5 << 4));   // data offset 5 (20 bytes), no options
    tcp.append(char(flags));
    putBE16(tcp, 65535);        // window
    putBE16(tcp, 0);            // checksum (Wireshark does not verify by default)
    putBE16(tcp, 0);            // urgent pointer
    tcp.append(payload);

    QByteArray ip;
    if (v6) {
        putBE32(ip, 0x60000000u);               // version 6, no TC/flow
        putBE16(ip, quint16(tcp.size()));        // payload length
        ip.append(char(6));                      // next header: TCP
        ip.append(char(64));                     // hop limit
        ip.append(src);
        ip.append(dst);
    } else {
        QByteArray h;
        h.append(char(0x45));                    // version 4, IHL 5
        h.append(char(0));                       // DSCP/ECN
        putBE16(h, quint16(20 + tcp.size()));    // total length
        putBE16(h, ipId);
        putBE16(h, 0x4000);                      // flags: DF
        h.append(char(64));                      // TTL
        h.append(char(6));                       // protocol: TCP
        putBE16(h, 0);                           // checksum placeholder
        h.append(src);
        h.append(dst);
        const quint16 csum = ipChecksum(h);
        h[10] = char(csum >> 8);
        h[11] = char(csum & 0xff);
        ip = h;
    }
    ip.append(tcp);

    QByteArray eth;
    const char cMac[6] = {0x02, 0, 0, 0, 0, 0x01};
    const char sMac[6] = {0x02, 0, 0, 0, 0, 0x02};
    eth.append(toServer ? QByteArray(sMac, 6) : QByteArray(cMac, 6));   // dst MAC
    eth.append(toServer ? QByteArray(cMac, 6) : QByteArray(sMac, 6));   // src MAC
    putBE16(eth, v6 ? 0x86DD : 0x0800);
    eth.append(ip);
    return eth;
}

void writeEpb(QByteArray &out, const QByteArray &frame, qint64 tsMs)
{
    const quint64 us = quint64(tsMs) * 1000;
    QByteArray body;
    putLE32(body, 0);                               // interface id
    putLE32(body, quint32(us >> 32));               // timestamp high
    putLE32(body, quint32(us & 0xffffffffu));       // timestamp low
    putLE32(body, quint32(frame.size()));           // captured length
    putLE32(body, quint32(frame.size()));           // original length
    body.append(frame);
    while (body.size() % 4)                          // pad packet data to 32 bits
        body.append(char(0));
    const quint32 total = 12 + quint32(body.size());
    putLE32(out, 0x00000006u);                       // Enhanced Packet Block
    putLE32(out, total);
    out.append(body);
    putLE32(out, total);
}

} // namespace

QByteArray Capture::toPcapng(const QList<CaptureEntry> &entries)
{
    // First pass: resolve the two endpoints of every connection and collect the
    // IP -> name pairs for the Name Resolution Block.
    struct Resolved {
        bool v6 = false;
        QByteArray cAddr, sAddr;
        quint16 cPort = 0, sPort = 0;
        qint64 t0 = 0;
    };
    QList<Resolved> resolved;
    resolved.reserve(entries.size());
    struct NrbRec { bool v6; QByteArray ip; QString name; };
    QList<NrbRec> nrb;
    auto addName = [&nrb](bool v6, const QByteArray &ip, const QString &name) {
        if (name.isEmpty())
            return;
        for (const NrbRec &r : nrb)
            if (r.v6 == v6 && r.ip == ip)
                return;
        nrb.append({v6, ip, name});
    };

    for (const CaptureEntry &e : entries) {
        const bool v6 = e.meta.srcAddr.protocol() == QAbstractSocket::IPv6Protocol
                        || e.meta.dstAddr.protocol() == QAbstractSocket::IPv6Protocol || e.meta.v6;
        auto pick = [&](const QHostAddress &a, const QString &seed, bool server) {
            if (isSpecific(a) && (a.protocol() == QAbstractSocket::IPv6Protocol) == v6)
                return ipBytesOf(a, v6);
            return synthIp(seed, v6, server);
        };
        Resolved r;
        r.v6 = v6;
        r.cAddr = pick(e.meta.srcAddr, e.meta.appName.isEmpty() ? QStringLiteral("client") : e.meta.appName, false);
        r.sAddr = pick(e.meta.dstAddr, e.meta.hostname.isEmpty() ? e.meta.target : e.meta.hostname, true);
        r.cPort = e.meta.srcPort ? e.meta.srcPort : 40000;
        r.sPort = e.meta.dstPort ? e.meta.dstPort : (e.proto == "TLS" ? 443 : 80);
        // For a decrypted connection the payload is plaintext, so writing it on
        // port 443 would make Wireshark run its TLS dissector over cleartext and
        // show a garbled, "incomplete" stream. Move it to a port whose default
        // dissector matches the plaintext: 80 for HTTP, otherwise a neutral port
        // with no dissector (Follow TCP Stream still shows the bytes).
        if (e.decrypted)
            r.sPort = (e.proto == "HTTP") ? 80 : (r.sPort != 443 ? r.sPort : 8000);
        r.t0 = e.meta.startMs ? e.meta.startMs : QDateTime::currentMSecsSinceEpoch();
        resolved.append(r);

        addName(v6, r.cAddr, e.meta.appName);
        const QString host = e.meta.hostname.isEmpty() ? e.meta.target.section(':', 0, -2) : e.meta.hostname;
        addName(v6, r.sAddr, host);
    }

    QByteArray out;

    // Section Header Block.
    {
        QByteArray body;
        putLE32(body, 0x1A2B3C4Du);   // byte-order magic
        putLE16(body, 1);             // major version
        putLE16(body, 0);             // minor version
        putLE64(body, 0xffffffffffffffffULL);   // section length: not specified
        const quint32 total = 12 + quint32(body.size());
        putLE32(out, 0x0A0D0D0Au);
        putLE32(out, total);
        out.append(body);
        putLE32(out, total);
    }
    // Interface Description Block (Ethernet).
    {
        QByteArray body;
        putLE16(body, 1);    // link type: LINKTYPE_ETHERNET
        putLE16(body, 0);    // reserved
        putLE32(body, 0);    // snap length: unlimited
        const quint32 total = 12 + quint32(body.size());
        putLE32(out, 0x00000001u);
        putLE32(out, total);
        out.append(body);
        putLE32(out, total);
    }
    // Name Resolution Block, mapping the placeholder/real IPs to names.
    if (!nrb.isEmpty()) {
        QByteArray body;
        for (const NrbRec &r : nrb)
            writeNrbRecord(body, r.v6, r.ip, r.name);
        putLE16(body, 0);   // nrb_record_end
        putLE16(body, 0);
        const quint32 total = 12 + quint32(body.size());
        putLE32(out, 0x00000004u);
        putLE32(out, total);
        out.append(body);
        putLE32(out, total);
    }

    quint16 ipId = 1;
    for (int ei = 0; ei < entries.size(); ++ei) {
        const CaptureEntry &e = entries[ei];
        const Resolved &r = resolved[ei];
        const bool v6 = r.v6;
        const QByteArray &cAddr = r.cAddr;
        const QByteArray &sAddr = r.sAddr;
        const quint16 cPort = r.cPort;
        const quint16 sPort = r.sPort;
        const qint64 t0 = r.t0;

        quint32 cSeq = 1000, sSeq = 2000;
        // Three-way handshake.
        writeEpb(out, makeFrame(v6, true, cAddr, sAddr, cPort, sPort, cSeq, 0, 0x02, {}, ipId++), t0);
        writeEpb(out, makeFrame(v6, false, cAddr, sAddr, cPort, sPort, sSeq, cSeq + 1, 0x12, {}, ipId++), t0);
        ++cSeq; ++sSeq;
        writeEpb(out, makeFrame(v6, true, cAddr, sAddr, cPort, sPort, cSeq, sSeq, 0x10, {}, ipId++), t0);

        for (const CaptureSegment &seg : e.segs) {
            if (seg.data.isEmpty())
                continue;
            if (seg.dir == CaptureDir::Up) {
                writeEpb(out, makeFrame(v6, true, cAddr, sAddr, cPort, sPort, cSeq, sSeq, 0x18, seg.data, ipId++), seg.tsMs);
                cSeq += quint32(seg.data.size());
            } else {
                writeEpb(out, makeFrame(v6, false, cAddr, sAddr, cPort, sPort, sSeq, cSeq, 0x18, seg.data, ipId++), seg.tsMs);
                sSeq += quint32(seg.data.size());
            }
        }
    }
    return out;
}

// ---- HAR export ------------------------------------------------------------

namespace {

QJsonArray harHeaders(const HttpMessage &m)
{
    QJsonArray a;
    for (const HttpHeader &h : m.headers) {
        QJsonObject o;
        o["name"] = h.name;
        o["value"] = h.value;
        a.append(o);
    }
    return a;
}

QJsonObject harContent(const HttpMessage &m)
{
    QJsonObject c;
    c["size"] = m.body.size();
    c["mimeType"] = m.header("Content-Type");
    if (!m.body.isEmpty()) {
        c["text"] = QString::fromLatin1(m.body.toBase64());
        c["encoding"] = "base64";
    }
    return c;
}

} // namespace

QByteArray Capture::toHar(const QList<CaptureEntry> &entries)
{
    QJsonArray harEntries;
    for (const CaptureEntry &e : entries) {
        const QList<HttpExchange> exchanges = parseHttp(e);
        for (const HttpExchange &ex : exchanges) {
            if (!ex.request.valid)
                continue;
            const QString scheme = (e.proto == "TLS" || e.meta.dstPort == 443) ? "https" : "http";
            const QString host = e.meta.hostname.isEmpty() ? e.meta.target : e.meta.hostname;
            QString url = ex.request.uri;
            if (url.startsWith('/'))
                url = scheme + "://" + host + url;

            QJsonObject req;
            req["method"] = ex.request.method;
            req["url"] = url;
            req["httpVersion"] = ex.request.version;
            req["headers"] = harHeaders(ex.request);
            req["queryString"] = QJsonArray();
            req["cookies"] = QJsonArray();
            req["headersSize"] = -1;
            req["bodySize"] = ex.request.body.size();
            if (!ex.request.body.isEmpty()) {
                QJsonObject pd;
                pd["mimeType"] = ex.request.header("Content-Type");
                pd["text"] = QString::fromLatin1(ex.request.body.toBase64());
                req["postData"] = pd;
            }

            QJsonObject resp;
            resp["status"] = ex.response.valid ? ex.response.statusCode : 0;
            resp["statusText"] = ex.response.reason;
            resp["httpVersion"] = ex.response.version;
            resp["headers"] = harHeaders(ex.response);
            resp["cookies"] = QJsonArray();
            resp["content"] = harContent(ex.response);
            resp["redirectURL"] = ex.response.header("Location");
            resp["headersSize"] = -1;
            resp["bodySize"] = ex.response.body.size();

            QJsonObject entry;
            entry["startedDateTime"] =
                QDateTime::fromMSecsSinceEpoch(ex.reqTsMs ? ex.reqTsMs : e.meta.startMs).toUTC().toString(Qt::ISODateWithMs);
            const qint64 ms = (ex.respTsMs && ex.reqTsMs) ? qMax<qint64>(0, ex.respTsMs - ex.reqTsMs) : 0;
            entry["time"] = double(ms);
            entry["request"] = req;
            entry["response"] = resp;
            entry["cache"] = QJsonObject();
            QJsonObject timings;
            timings["send"] = 0;
            timings["wait"] = double(ms);
            timings["receive"] = 0;
            entry["timings"] = timings;
            if (!e.meta.appName.isEmpty())
                entry["comment"] = e.meta.appName;
            harEntries.append(entry);
        }
    }

    QJsonObject creator;
    creator["name"] = "WinRoute";
#ifdef WINROUTE_VERSION
    creator["version"] = QStringLiteral(WINROUTE_VERSION);
#else
    creator["version"] = QString();
#endif
    QJsonObject log;
    log["version"] = "1.2";
    log["creator"] = creator;
    log["entries"] = harEntries;
    QJsonObject root;
    root["log"] = log;
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}
