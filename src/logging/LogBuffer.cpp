#include "logging/LogBuffer.h"

#include <QJsonObject>

namespace zarya {

LogBuffer& LogBuffer::instance()
{
    static LogBuffer buffer;
    return buffer;
}

void LogBuffer::setAppStartedAt(const QDateTime& startedAt)
{
    m_appStartedAt = startedAt;
}

QDateTime LogBuffer::appStartedAt() const
{
    return m_appStartedAt;
}

void LogBuffer::append(const QString& line)
{
    if (line.isEmpty()) {
        return;
    }
    m_lines.append(line);
    while (m_lines.size() > kMaxStoredLines) {
        m_lines.removeFirst();
    }

    const QString lower = line.toLower();
    if (lower.contains(QStringLiteral("error")) || lower.contains(QStringLiteral("failed"))
        || lower.contains(QStringLiteral("fatal"))) {
        appendError(QStringLiteral("Log"), line);
    }
}

void LogBuffer::appendError(const QString& area, const QString& message)
{
    LogErrorEntry entry;
    entry.timestamp = QDateTime::currentDateTimeUtc();
    entry.area = area;
    entry.message = message;
    m_errors.append(entry);
    while (m_errors.size() > kMaxStoredErrors) {
        m_errors.removeFirst();
    }
}

QStringList LogBuffer::recentLines(int maxLines) const
{
    if (maxLines <= 0 || m_lines.isEmpty()) {
        return {};
    }
    return m_lines.mid(qMax(0, m_lines.size() - maxLines));
}

QVector<LogErrorEntry> LogBuffer::recentErrors(int maxEntries) const
{
    if (maxEntries <= 0 || m_errors.isEmpty()) {
        return {};
    }
    return m_errors.mid(qMax(0, m_errors.size() - maxEntries));
}

QJsonArray LogBuffer::recentErrorsJson(int maxEntries) const
{
    QJsonArray array;
    for (const LogErrorEntry& entry : recentErrors(maxEntries)) {
        QJsonObject object;
        object.insert(QStringLiteral("timestamp"), entry.timestamp.toString(Qt::ISODate));
        object.insert(QStringLiteral("area"), entry.area);
        object.insert(QStringLiteral("message"), entry.message);
        array.append(object);
    }
    return array;
}

} // namespace zarya
