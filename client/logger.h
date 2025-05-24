#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDate>

enum LogLevel
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
};

class logger
{
public:
    static logger& instance();

    void log(const QString& message, LogLevel level = LOG_INFO);
    void setLogDirectory(const QString& path);

private:
    logger();
    ~logger();

    void openLogFile();
    void rotateLogFileIfNeeded();
    QString generateLogFileName(int index = 0) const;
    QString logLevelToString(LogLevel level);

    QFile logFile;
    QTextStream logStream;
    QMutex mutex;

    QString logDirectory;
    QDate currentDate;
    int currentIndex;
};

#define LOG_INFO_MSG(msg) logger::instance().log(msg, LOG_INFO)
#define LOG_WARN_MSG(msg) logger::instance().log(msg, LOG_WARNING)
#define LOG_ERROR_MSG(msg) logger::instance().log(msg, LOG_ERROR)
#define LOG_DEBUG_MSG(msg) logger::instance().log(msg, LOG_DEBUG)

#endif // LOGGER_H
