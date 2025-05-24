#include "logger.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

const qint64 MAX_LOG_FILE_SIZE = 100 * 1024 * 1024;  // 100MB

logger::logger()
    : logDirectory(QDir::currentPath() + "/logs"),
      currentDate(QDate::currentDate()),
      currentIndex(0)
{
    QDir().mkpath(logDirectory);
    openLogFile();
}

logger::~logger()
{
    if (logFile.isOpen())
        logFile.close();
}

logger& logger::instance()
{
    static logger instance;
    return instance;
}

void logger::setLogDirectory(const QString& path)
{
    QMutexLocker locker(&mutex);
    logDirectory = path;
    QDir().mkpath(logDirectory);
    openLogFile();
}

void logger::openLogFile()
{
    if (logFile.isOpen()) logFile.close();

    // 查找当前日期已有多少个文件
    QString baseName = currentDate.toString("yyyy-MM-dd");
    currentIndex = 0;
    while (QFile::exists(generateLogFileName(currentIndex)))
        currentIndex++;

    QString filePath = generateLogFileName(currentIndex);
    logFile.setFileName(filePath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    logStream.setDevice(&logFile);
}

QString logger::generateLogFileName(int index) const
{
    QString base = logDirectory + "/" + currentDate.toString("yyyy-MM-dd");
    if (index == 0)
        return base + ".log";
    else
        return base + "_" + QString::number(index) + ".log";
}

void logger::rotateLogFileIfNeeded()
{
    QDate today = QDate::currentDate();
    if (today != currentDate) {
        currentDate = today;
        currentIndex = 0;
        openLogFile();
    }

    QFileInfo info(logFile);
    if (info.size() >= MAX_LOG_FILE_SIZE) {
        currentIndex++;
        openLogFile();  // 自动写入新的编号文件
    }
}

QString logger::logLevelToString(LogLevel level)
{
    switch (level)
    {
        case LOG_INFO: return "[INFO]";
        case LOG_WARNING: return "[WARNING]";
        case LOG_ERROR: return "[ERROR]";
        case LOG_DEBUG: return "[DEBUG]";
        default: return "[UNKNOWN]";
    }
}

void logger::log(const QString& message, LogLevel level)
{
    QMutexLocker locker(&mutex);

    rotateLogFileIfNeeded();

    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    logStream << timeStamp << " " << logLevelToString(level) << " " << message << "\n";
    logStream.flush();
}
