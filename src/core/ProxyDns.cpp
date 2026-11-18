#include "ProxyDns.h"
#include "DnsMessage.h"
#include "ProxyConnector.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QPointer>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

namespace {
constexpr int kTimeoutMs = 8000;
constexpr quint32 kMinTtl = 30, kMaxTtl = 600;
}

ProxyDns::ProxyDns(DnsClient *localDns, QObject *parent)
    : QObject(parent), m_localDns(localDns)
{
}

QList<QList<ProxyServer>> ProxyDns::routes(const Profile &profile, int id)
{
    if (const ProxyServer *p = profile.proxyById(id))
        return {{*p}};
    const ProxyChain *c = profile.chainById(id);
    if (!c)
        return {};
    QList<ProxyServer> members;
    for (const ChainMember &m : c->members)
        if (m.enabled)
            if (const ProxyServer *p = profile.proxyById(m.proxyId))
                members << *p;
    if (members.isEmpty())
        return {};
    if (c->type == ChainType::Simple)
        return {members};
    // Redundancy: the members in order. Load balancing: from a random one on.
    QList<QList<ProxyServer>> out;
    const int start = c->type == ChainType::LoadBalancing ? int(QRandomGenerator::global()->bounded(members.size())) : 0;
    for (int i = 0; i < members.size(); ++i)
        out << QList<ProxyServer>{members[(start + i) % members.size()]};
    return out;
}

bool ProxyDns::parseServer(const QString &text, QHostAddress *addr, quint16 *port)
{
    const QString t = text.trimmed();
    *port = 53;
    QString host = t;
    if (t.startsWith(u'[')) {   // [v6]:port
        const int close = t.indexOf(u']');
        if (close < 0)
            return false;
        host = t.mid(1, close - 1);
        const QString rest = t.mid(close + 1);
        if (!rest.isEmpty()) {
            bool ok = false;
            const uint p = rest.startsWith(u':') ? rest.mid(1).toUInt(&ok) : 0;
            if (!ok || p == 0 || p > 65535)
                return false;
            *port = quint16(p);
        }
    } else if (t.count(u':') == 1) {   // v4:port
        const int colon = t.indexOf(u':');
        host = t.left(colon);
        bool ok = false;
        const uint p = t.mid(colon + 1).toUInt(&ok);
        if (!ok || p == 0 || p > 65535)
            return false;
        *port = quint16(p);
    }
    *addr = QHostAddress(host);
    return !addr->isNull();
}

QString ProxyDns::resolverName(const Profile &profile, int id)
{
    if (const ProxyServer *p = profile.proxyById(id))
        return QCoreApplication::translate("ProxyDns", "proxy %1").arg(p->displayName());
    if (const ProxyChain *c = profile.chainById(id))
        return QCoreApplication::translate("ProxyDns", "chain %1").arg(c->name);
    return QCoreApplication::translate("ProxyDns", "proxy #%1").arg(id);
}

void ProxyDns::resolve(const Profile &profile, int resolverId, const QString &server, const QString &name, Callback cb)
{
    const QString key = QString::number(resolverId) + u'/' + server + u'/' + name.toLower();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const auto cached = m_cache.constFind(key);
    if (cached != m_cache.cend() && cached->second > now) {
        cb(cached->first, QString());
        return;
    }
    const bool started = m_waiting.contains(key);
    m_waiting[key] << cb;
    if (started)
        return;

    Job job;
    job.routes = routes(profile, resolverId);
    job.name = name;
    job.key = key;
    if (!parseServer(server, &job.server, &job.port)) {
        finish(key, {}, QCoreApplication::translate("ProxyDns", "invalid DNS server address %1").arg(server), 0);
        return;
    }
    if (job.routes.isEmpty()) {
        finish(key, {}, QCoreApplication::translate("ProxyDns", "%1 does not exist or has no enabled proxy servers")
                            .arg(resolverName(profile, resolverId)), 0);
        return;
    }
    attempt(job);
}

void ProxyDns::attempt(Job job)
{
    if (job.route >= job.routes.size()) {
        finish(job.key, {}, job.lastError, 0);
        return;
    }
    auto *socket = new QTcpSocket(this);
    auto *connector = new ProxyConnector(socket, job.routes[job.route], Endpoint{{}, job.server, job.port}, m_localDns, kTimeoutMs, socket);
    const quint16 id = m_nextId++;
    auto buffer = std::make_shared<QByteArray>();
    auto done = std::make_shared<bool>(false);   // the first outcome wins
    QPointer<ProxyDns> self(this);

    auto fail = [self, socket, job, done](const QString &err) mutable {
        if (*done)
            return;
        *done = true;
        socket->disconnect();
        socket->deleteLater();
        if (!self)
            return;
        job.lastError = err;
        ++job.route;
        self->attempt(job);
    };
    auto onData = [self, socket, job, id, buffer, done]() mutable {
        if (*done || buffer->size() < 2)
            return;
        const int len = qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(buffer->constData()));
        if (buffer->size() < 2 + len)
            return;
        const QByteArray msg = buffer->mid(2, len);
        *done = true;
        socket->disconnect();
        socket->deleteLater();
        if (!self)
            return;
        dns::Question q;
        QList<dns::Answer> answers;
        if (!dns::parseAnswers(msg, &q, &answers) || q.id != id) {
            job.lastError = QCoreApplication::translate("ProxyDns", "invalid DNS response");
            ++job.route;
            self->attempt(job);
            return;
        }
        QList<QHostAddress> addrs;
        quint32 ttl = kMaxTtl;
        for (const dns::Answer &a : answers) {
            if (a.type == dns::TypeA && !a.address.isNull()) {
                addrs << a.address;
                ttl = qMin(ttl, a.ttl);
            }
        }
        const int rcode = q.flags & 0x0F;
        QString err;
        if (addrs.isEmpty())
            err = rcode == 3 ? QCoreApplication::translate("ProxyDns", "host not found")
                             : QCoreApplication::translate("ProxyDns", "no address in the DNS response");
        self->finish(job.key, addrs, err, ttl);
    };

    connect(connector, &ProxyConnector::connected, socket, [socket, job, id, buffer, onData](const QByteArray &leftover) mutable {
        *buffer = leftover;
        const QByteArray query = dns::buildQuery(id, job.name, dns::TypeA);
        QByteArray framed(2, '\0');
        qToBigEndian<quint16>(quint16(query.size()), reinterpret_cast<uchar *>(framed.data()));
        socket->write(framed + query);
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, buffer, onData]() mutable {
            *buffer += socket->readAll();
            onData();
        });
        onData();
    });
    connect(connector, &ProxyConnector::failed, socket, [fail](const QString &err) mutable { fail(err); });
    // The tunnel is up but the server does not answer, or closes.
    auto *timer = new QTimer(socket);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, socket, [fail]() mutable {
        fail(QCoreApplication::translate("ProxyDns", "DNS request timed out"));
    });
    timer->start(kTimeoutMs * 2);
    connect(socket, &QTcpSocket::disconnected, socket, [fail]() mutable {
        fail(QCoreApplication::translate("ProxyDns", "connection closed by the DNS server"));
    });
    connector->start();
}

void ProxyDns::finish(const QString &key, const QList<QHostAddress> &addrs, const QString &error, quint32 ttl)
{
    if (!addrs.isEmpty())
        m_cache.insert(key, {addrs, QDateTime::currentMSecsSinceEpoch() + qint64(qBound(kMinTtl, ttl, kMaxTtl)) * 1000});
    const QList<Callback> cbs = m_waiting.take(key);
    for (const Callback &cb : cbs)
        cb(addrs, error);
}
