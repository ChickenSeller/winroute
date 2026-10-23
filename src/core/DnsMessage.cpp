#include "DnsMessage.h"

#include <QUrl>
#include <QtEndian>

namespace dns {

static quint16 rd16(const QByteArray &m, int off)
{
    return qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(m.constData()) + off);
}

static quint32 rd32(const QByteArray &m, int off)
{
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m.constData()) + off);
}

// Reads a (possibly compressed) name starting at *off; advances *off past it.
static bool readName(const QByteArray &m, int *off, QString *out)
{
    QStringList labels;
    int pos = *off;
    bool jumped = false;
    int hops = 0;
    while (true) {
        if (pos >= m.size())
            return false;
        const quint8 len = quint8(m[pos]);
        if ((len & 0xC0) == 0xC0) {
            if (pos + 1 >= m.size() || ++hops > 32)
                return false;
            const int ptr = ((len & 0x3F) << 8) | quint8(m[pos + 1]);
            if (!jumped)
                *off = pos + 2;
            jumped = true;
            pos = ptr;
            continue;
        }
        if (len == 0) {
            if (!jumped)
                *off = pos + 1;
            break;
        }
        if (pos + 1 + len > m.size())
            return false;
        labels << QString::fromLatin1(m.constData() + pos + 1, len);
        pos += 1 + len;
    }
    *out = labels.join(u'.');
    return true;
}

bool parseQuestion(const QByteArray &m, Question *q)
{
    if (m.size() < 12)
        return false;
    q->id = rd16(m, 0);
    q->flags = rd16(m, 2);
    if (rd16(m, 4) < 1)
        return false;
    int off = 12;
    if (!readName(m, &off, &q->name) || off + 4 > m.size())
        return false;
    q->qtype = rd16(m, off);
    q->qclass = rd16(m, off + 2);
    q->questionEnd = off + 4;
    return true;
}

bool parseAnswers(const QByteArray &m, Question *q, QList<Answer> *answers)
{
    if (!parseQuestion(m, q))
        return false;
    const int qdcount = rd16(m, 4);
    const int ancount = rd16(m, 6);
    int off = 12;
    for (int i = 0; i < qdcount; ++i) {
        QString n;
        if (!readName(m, &off, &n))
            return false;
        off += 4;
    }
    for (int i = 0; i < ancount; ++i) {
        Answer a;
        if (!readName(m, &off, &a.name) || off + 10 > m.size())
            return false;
        a.type = rd16(m, off);
        a.ttl = rd32(m, off + 4);
        const int rdlen = rd16(m, off + 8);
        off += 10;
        if (off + rdlen > m.size())
            return false;
        if (a.type == TypeA && rdlen == 4) {
            a.address = QHostAddress(rd32(m, off));
        } else if (a.type == TypeAAAA && rdlen == 16) {
            a.address = QHostAddress(reinterpret_cast<const quint8 *>(m.constData() + off));
        } else if (a.type == TypeCNAME) {
            int o = off;
            readName(m, &o, &a.target);
        }
        off += rdlen;
        answers->append(a);
    }
    return true;
}

static void put16(QByteArray &b, quint16 v)
{
    b.append(char(v >> 8));
    b.append(char(v & 0xFF));
}

static void put32(QByteArray &b, quint32 v)
{
    put16(b, quint16(v >> 16));
    put16(b, quint16(v & 0xFFFF));
}

QByteArray buildResponse(const QByteArray &query, const Question &q, const QList<QHostAddress> &addrs,
                         quint32 ttl, quint16 rcode)
{
    QByteArray r;
    put16(r, q.id);
    // QR=1, opcode from query, AA=0, RD copied, RA=1
    put16(r, quint16(0x8000 | (q.flags & 0x7900) | 0x0080 | (rcode & 0xF)));
    put16(r, 1);
    put16(r, quint16(addrs.size()));
    put16(r, 0);
    put16(r, 0);
    r.append(query.mid(12, q.questionEnd - 12));
    for (const QHostAddress &a : addrs) {
        put16(r, 0xC00C);   // pointer to the question name
        const bool v4 = a.protocol() == QAbstractSocket::IPv4Protocol;
        put16(r, v4 ? TypeA : TypeAAAA);
        put16(r, 1);
        put32(r, ttl);
        if (v4) {
            put16(r, 4);
            put32(r, a.toIPv4Address());
        } else {
            put16(r, 16);
            const Q_IPV6ADDR v6 = a.toIPv6Address();
            r.append(reinterpret_cast<const char *>(&v6), 16);
        }
    }
    return r;
}

QByteArray buildQuery(quint16 id, const QString &name, quint16 qtype)
{
    QByteArray r;
    put16(r, id);
    put16(r, 0x0100);   // RD
    put16(r, 1);
    put16(r, 0);
    put16(r, 0);
    put16(r, 0);
    for (const QString &label : name.split(u'.', Qt::SkipEmptyParts)) {
        const QByteArray l = QUrl::toAce(label).left(63);
        r.append(char(l.size()));
        r.append(l);
    }
    r.append('\0');
    put16(r, qtype);
    put16(r, 1);
    return r;
}

} // namespace dns
