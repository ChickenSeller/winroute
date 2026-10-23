#include "Utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>

#include <windows.h>

namespace util {

bool wildcardMatch(QStringView pattern, QStringView text)
{
    // Iterative glob with backtracking on the last '*'.
    qsizetype p = 0, t = 0, star = -1, mark = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == u'?' || pattern[p].toCaseFolded() == text[t].toCaseFolded())) {
            ++p;
            ++t;
        } else if (p < pattern.size() && pattern[p] == u'*') {
            star = p++;
            mark = t;
        } else if (star >= 0) {
            p = star + 1;
            t = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == u'*')
        ++p;
    return p == pattern.size();
}

QStringList splitList(const QString &list)
{
    QStringList out;
    QString cur;
    bool quoted = false;
    for (QChar c : list) {
        if (c == u'"') {
            quoted = !quoted;
        } else if (!quoted && (c == u';' || c == u',' || c == u'\n' || c == u'\r')) {
            cur = cur.trimmed();
            if (!cur.isEmpty())
                out << cur;
            cur.clear();
        } else {
            cur += c;
        }
    }
    cur = cur.trimmed();
    if (!cur.isEmpty())
        out << cur;
    return out;
}

QString computerName()
{
    static const QString name = [] {
        wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(buf, &len))
            return QString::fromWCharArray(buf, int(len));
        return QString();
    }();
    return name;
}

QString dataDir()
{
    return QDir(QDir::fromNativeSeparators(qEnvironmentVariable("APPDATA"))).filePath("WinRoute");
}

QString expandEnv(const QString &s)
{
    if (!s.contains(u'%'))
        return s;
    static const QRegularExpression re("%([^%]+)%");
    QString out;
    qsizetype last = 0;
    auto it = re.globalMatch(s);
    while (it.hasNext()) {
        auto m = it.next();
        out += s.mid(last, m.capturedStart() - last);
        const QString var = m.captured(1);
        QString val;
        if (var.compare("ComputerName", Qt::CaseInsensitive) == 0)
            val = computerName();
        else
            val = qEnvironmentVariable(var.toLocal8Bit().constData());
        out += val.isEmpty() ? m.captured(0) : val;
        last = m.capturedEnd();
    }
    out += s.mid(last);
    return out;
}

QString formatBytes(qint64 bytes)
{
    if (bytes < 1024)
        return QCoreApplication::translate("Units", "%1 bytes").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    if (bytes < 1024LL * 1024 * 1024)
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 2);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}

QString formatRate(double bps)
{
    if (bps < 1024)
        return QCoreApplication::translate("Units", "%1 B/sec").arg(qRound(bps));
    if (bps < 1024 * 1024)
        return QCoreApplication::translate("Units", "%1 KB/sec").arg(bps / 1024.0, 0, 'f', 1);
    return QCoreApplication::translate("Units", "%1 MB/sec").arg(bps / (1024.0 * 1024), 0, 'f', 2);
}

QString formatDuration(qint64 s)
{
    if (s < 3600)
        return QString("%1:%2").arg(s / 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
    return QString("%1:%2:%3")
        .arg(s / 3600, 2, 10, QChar('0'))
        .arg((s / 60) % 60, 2, 10, QChar('0'))
        .arg(s % 60, 2, 10, QChar('0'));
}

QHostAddress normalize(const QHostAddress &addr)
{
    bool ok = false;
    const quint32 v4 = addr.toIPv4Address(&ok);
    if (ok && addr.protocol() == QAbstractSocket::IPv6Protocol)
        return QHostAddress(v4);
    return addr;
}

QString formatEndpoint(const QHostAddress &addr, quint16 port)
{
    const QHostAddress a = normalize(addr);
    if (a.protocol() == QAbstractSocket::IPv6Protocol)
        return QString("[%1]:%2").arg(a.toString()).arg(port);
    return QString("%1:%2").arg(a.toString()).arg(port);
}

} // namespace util
