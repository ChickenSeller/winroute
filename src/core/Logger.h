#pragma once

#include <QFile>
#include <QMutex>
#include <QObject>

enum class LogLevel { Error = 0, Normal = 1, Verbose = 2, Debug = 3 };

// Process-wide log sink. Thread-safe; the UI subscribes to entryAdded().
class Logger : public QObject {
    Q_OBJECT
public:
    static Logger &instance();

    void log(LogLevel level, const QString &text);

    void setFileLevel(int level);   // -1 disables the log file
    int fileLevel() const { return m_fileLevel; }
    QString filePath() const;

signals:
    void entryAdded(int level, const QString &line);

private:
    Logger() = default;
    QMutex m_mutex;
    QFile m_file;
    int m_fileLevel = -1;
};

inline void logError(const QString &s) { Logger::instance().log(LogLevel::Error, s); }
inline void logNormal(const QString &s) { Logger::instance().log(LogLevel::Normal, s); }
inline void logVerbose(const QString &s) { Logger::instance().log(LogLevel::Verbose, s); }
inline void logDebug(const QString &s) { Logger::instance().log(LogLevel::Debug, s); }
