#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QString>

// Minimal DNS wire-format helpers (RFC 1035) for the fake DNS responder,
// the response sniffer and the relay's own resolver.
namespace dns {

enum : quint16 { TypeA = 1, TypeCNAME = 5, TypeAAAA = 28, TypeHTTPS = 65 };

struct Question {
    quint16 id = 0;
    quint16 flags = 0;
    QString name;
    quint16 qtype = 0;
    quint16 qclass = 1;
    int questionEnd = 0;   // offset after the first question
};

struct Answer {
    QString name;
    quint16 type = 0;
    quint32 ttl = 0;
    QHostAddress address;  // A / AAAA
    QString target;        // CNAME
};

bool parseQuestion(const QByteArray &msg, Question *q);
bool parseAnswers(const QByteArray &msg, Question *q, QList<Answer> *answers);

// Response echoing the question with the given A/AAAA records (may be empty).
QByteArray buildResponse(const QByteArray &query, const Question &q, const QList<QHostAddress> &addrs,
                         quint32 ttl, quint16 rcode = 0);
QByteArray buildQuery(quint16 id, const QString &name, quint16 qtype);

} // namespace dns
