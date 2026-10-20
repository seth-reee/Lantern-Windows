#include "diagnosticlog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThread>

namespace {
QMutex logMutex;
QFile logFile;
}

QString startDiagnosticLog()
{
    QMutexLocker lock(&logMutex);
    QString directory;
#ifdef Q_OS_WIN
    directory = QDir(qEnvironmentVariable("LOCALAPPDATA")).filePath("Lantern");
#else
    directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#endif
    if (!QDir().mkpath(directory))
        return {};
    logFile.setFileName(QDir(directory).filePath("diagnostics.log"));
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return {};
    logFile.write((QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                   + " diagnostic logging started\n").toUtf8());
    logFile.flush();
    return logFile.fileName();
}

void diagnosticLog(const QString &message)
{
    QMutexLocker lock(&logMutex);
    if (!logFile.isOpen())
        return;
    const auto threadId = quintptr(QThread::currentThreadId());
    const QString line = QStringLiteral("%1 [thread %2] %3\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
        .arg(threadId, 0, 16)
        .arg(message);
    logFile.write(line.toUtf8());
    logFile.flush();
}
