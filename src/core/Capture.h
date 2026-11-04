#pragma once

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>

#include <atomic>

// Optional packet capture for connections that go through the local relay.
// Off by default; when on, the relay feeds every byte of each connection's
// application-level stream here (plaintext once Phase 2 MITM decrypts TLS).
// Datagram flows are captured too: DNS queries and answers seen by the
// interceptor, and UDP tunnelled through SOCKS5 (see datagram()).
// Everything stays on this machine: nothing is reported or sent anywhere.

enum class CaptureDir { Up, Down };   // Up: client -> server, Down: server -> client

// One capture-filter rule. Match fields use WinRoute's rule syntax (semicolon
// lists, wildcards, IP ranges, port ranges); an empty field does not constrain.
// A rule matches a connection when every non-empty field matches (a rule with
// no fields at all never matches).
struct CaptureRule {
    bool enabled = true;
    QString applications;   // e.g. "chrome.exe; firefox.exe"
    QString targets;        // e.g. "*.example.com; 10.0.0.0-10.255.255.255"
    QString ports;          // e.g. "80; 443; 8000-9000"
};

// Which connections to capture: a list of rules plus how to apply them. A
// connection "matches the list" when any enabled rule matches it.
struct CaptureFilter {
    enum Mode { All, Only, Except };   // All: everything; Only: whitelist; Except: blacklist
    Mode mode = All;
    QList<CaptureRule> rules;
};

// One run of bytes in one direction, with the time it was seen.
struct CaptureSegment {
    CaptureDir dir = CaptureDir::Up;
    qint64 tsMs = 0;
    QByteArray data;
};

// Everything the capture knows about one connection, minus the payload.
struct CaptureMeta {
    quint64 id = 0;
    quint32 pid = 0;
    QString appName;
    QString appPath;
    QString target;        // "host:port" for display
    QString ruleName;
    QString actionText;    // "Direct", "1.2.3.4:1080 SOCKS5", "Chain Office"
    QHostAddress srcAddr;  // application-side endpoint (real)
    quint16 srcPort = 0;
    QHostAddress dstAddr;  // original destination (may be empty for a fake-DNS IP)
    quint16 dstPort = 0;
    QString hostname;      // target host name when known
    bool v6 = false;
    bool udp = false;      // a datagram flow (UDP/DNS): one segment per datagram
    qint64 startMs = 0;
};

struct CaptureEntry {
    CaptureMeta meta;
    QList<CaptureSegment> segs;
    qint64 up = 0;         // total bytes client -> server
    qint64 down = 0;       // total bytes server -> client
    qint64 endMs = 0;
    bool open = true;
    bool truncated = false;  // hit the per-connection cap; some payload was dropped
    bool decrypted = false;  // bytes are decrypted TLS plaintext (Phase 2)
    bool synthesized = false;  // a DNS answer WinRoute made itself (resolve through proxy)
    QString proto;           // "HTTP", "TLS", "TCP" (guessed from the first bytes); "DNS", "UDP"
    QString flowKey;         // datagram flows: the key datagram() groups them by
};

// Compact row for the list view; no payload copy.
struct CaptureSummary {
    CaptureMeta meta;
    qint64 up = 0, down = 0;
    qint64 endMs = 0;
    bool open = true;
    bool truncated = false;
    bool decrypted = false;
    bool synthesized = false;
    QString proto;
};

class Capture : public QObject {
    Q_OBJECT
public:
    explicit Capture(QObject *parent = nullptr);

    // Thread-safe. Toggling off keeps what was already captured.
    void setEnabled(bool on);
    bool isEnabled() const;

    // HTTPS decryption (MITM). When on, the relay terminates TLS for the
    // connections it captures and feeds this store the plaintext. Off by
    // default; read without the lock on the relay's fast path.
    void setDecrypt(bool on);
    bool decrypt() const;

    // Per-connection and total byte caps (thread-safe). 0 = unlimited.
    void setLimits(qint64 perConn, qint64 total);

    // Which connections to capture (thread-safe).
    void setFilter(const CaptureFilter &filter);
    CaptureFilter filter() const;
    // Whether a connection passes the filter. Called from the relay thread.
    bool shouldCapture(const QString &appName, const QString &appPath, const QString &host,
                       const QHostAddress &ip, quint16 port) const;

    // Fed from the relay thread.
    void note(const CaptureMeta &meta);
    void append(quint64 id, CaptureDir dir, const QByteArray &bytes, qint64 tsMs, bool decrypted = false);
    void finish(quint64 id, qint64 tsMs);

    // Records one datagram of a UDP flow, from any thread. Datagrams are grouped
    // into one entry per (process, local endpoint, remote endpoint, protocol);
    // the entry is created on the first datagram that passes the filter, with an
    // id assigned here (meta.id is ignored). `proto` is "DNS" or "UDP". Set
    // `synthesized` for a DNS answer WinRoute made up (fake DNS), so the view
    // can say the real resolution happened at the proxy. `filterHost`, when set,
    // is the host name the capture filter matches against (for DNS: the name
    // asked for), instead of meta.hostname.
    void datagram(const CaptureMeta &meta, const QString &proto, CaptureDir dir, const QByteArray &bytes,
                  qint64 tsMs, bool synthesized = false, const QString &filterHost = {});

    // A DNS message (query Up, answer Down) seen by an interceptor. `base` holds
    // the process and the two endpoints (application socket, DNS server); the
    // target text and the filter's host name come from the question.
    void dnsMessage(const CaptureMeta &base, CaptureDir dir, const QByteArray &msg, qint64 tsMs,
                    bool synthesized = false);

    // Reads for the UI (thread-safe snapshots).
    QList<CaptureSummary> summaries() const;
    CaptureEntry entry(quint64 id) const;
    bool hasEntry(quint64 id) const;
    int count() const;
    void clear();

    // Exports. Pure functions over a copied entry, so they are easy to test.
    // keepRealPorts: when false (default), a decrypted connection is written on
    // an HTTP-friendly port (80) so Wireshark dissects the plaintext as HTTP
    // rather than mis-reading it as TLS on 443. When true, the real ports are
    // kept and the reader must map the port to HTTP ("Decode As" in Wireshark).
    static QByteArray toPcapng(const QList<CaptureEntry> &entries, bool keepRealPorts = false);
    static QByteArray toHar(const QList<CaptureEntry> &entries);

    // A PCAPNG in Wireshark's "Exported PDU" link type: each direction's bytes
    // become a PDU tagged with its dissector (http/tls/data) and the real
    // addresses and ports. Wireshark then shows the payload as HTTP while still
    // displaying the true port (443), with no "Decode As" needed. There is no
    // TCP-stream/handshake view in this format.
    static QByteArray toExportedPdu(const QList<CaptureEntry> &entries);

signals:
    void changed();   // a connection was added or finished, or the store was cleared

private:
    void evictLocked();   // drop oldest entries while over the total or count cap

    mutable QMutex m_mutex;
    std::atomic<bool> m_enabled{false};   // read without the lock for a cheap off-path
    std::atomic<bool> m_decrypt{false};   // HTTPS decryption (MITM)
    qint64 m_perConn = 8LL * 1024 * 1024;
    qint64 m_total = 256LL * 1024 * 1024;
    qint64 m_bytes = 0;
    QHash<quint64, CaptureEntry> m_entries;
    QList<quint64> m_order;   // insertion order, oldest first (for eviction)
    QHash<QString, quint64> m_flows;   // datagram flow key -> entry id
    quint64 m_nextFlowId = 0;          // datagram entry ids: top bit set, apart from relay ids
    CaptureFilter m_filter;   // which connections to capture
};
