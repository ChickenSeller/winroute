#include "FlowTable.h"

#include <QDateTime>

void FlowTable::add(const RedirectInfo &info)
{
    QMutexLocker lock(&m_mutex);
    m_pending.insert(info.srcPort, info);
}

std::optional<RedirectInfo> FlowTable::take(quint16 appLocalPort)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_pending.find(appLocalPort);
    if (it == m_pending.end())
        return std::nullopt;
    RedirectInfo info = it.value();
    m_pending.erase(it);
    return info;
}

void FlowTable::expire(qint64 olderThanMs)
{
    const qint64 limit = QDateTime::currentMSecsSinceEpoch() - olderThanMs;
    QMutexLocker lock(&m_mutex);
    for (auto it = m_pending.begin(); it != m_pending.end();) {
        if (it->createdMs < limit)
            it = m_pending.erase(it);
        else
            ++it;
    }
}

void FlowTable::clear()
{
    QMutexLocker lock(&m_mutex);
    m_pending.clear();
}
