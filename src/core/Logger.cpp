#include "Logger.h"
#include "Utils.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

Logger &Logger::instance()
{
    static Logger *logger = new Logger;
    return *logger;
}

QString Logger::filePath() const
{
    return QDir(util::dataDir()).filePath("Log.txt");
}

void Logger::setFileLevel(int level)
{
    QMutexLocker lock(&m_mutex);
    m_fileLevel = level;
    if (level < 0) {
        m_file.close();
        return;
    }
    if (!m_file.isOpen()) {
        QDir().mkpath(QFileInfo(filePath()).path());
        m_file.setFileName(filePath());
        m_file.open(QIODevice::Append | QIODevice::Text);
    }
}

void Logger::log(LogLevel level, const QString &text)
{
    const QString line = QString("[%1] %2").arg(QDateTime::currentDateTime().toString("MM.dd HH:mm:ss"), text);
    {
        QMutexLocker lock(&m_mutex);
        if (m_fileLevel >= int(level) && m_file.isOpen()) {
            m_file.write(line.toUtf8());
            m_file.write("\n");
            m_file.flush();
        }
    }
    emit entryAdded(int(level), line);
}
