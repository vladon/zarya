#include "updater/runner/UpdaterLog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace zarya {

UpdaterLog::UpdaterLog(const QString& logFilePath)
    : m_logFilePath(logFilePath)
{
    QDir().mkpath(QFileInfo(logFilePath).absolutePath());
}

void UpdaterLog::writeLine(const QString& level, const QString& message)
{
    QFile file(m_logFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream stream(&file);
    stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << ' ' << level << ' '
           << message << '\n';
}

void UpdaterLog::info(const QString& message)
{
    writeLine(QStringLiteral("INFO"), message);
}

void UpdaterLog::warning(const QString& message)
{
    writeLine(QStringLiteral("WARN"), message);
}

void UpdaterLog::error(const QString& message)
{
    writeLine(QStringLiteral("ERROR"), message);
}

QString UpdaterLog::logFilePath() const
{
    return m_logFilePath;
}

} // namespace zarya
