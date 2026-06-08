#pragma once

#include "app/StartupOptions.h"

#include <QDateTime>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace zarya {

struct LogErrorEntry {
    QDateTime timestamp;
    QString area;
    QString message;
};

class LogBuffer {
public:
    static LogBuffer& instance();

    void setAppStartedAt(const QDateTime& startedAt);
    QDateTime appStartedAt() const;

    void setMinLogLevel(LogLevel level);
    LogLevel minLogLevel() const;

    void append(const QString& line);
    void appendError(const QString& area, const QString& message);

    QStringList recentLines(int maxLines) const;
    QVector<LogErrorEntry> recentErrors(int maxEntries = 50) const;
    QJsonArray recentErrorsJson(int maxEntries = 50) const;

private:
    LogBuffer() = default;

    QStringList m_lines;
    QVector<LogErrorEntry> m_errors;
    QDateTime m_appStartedAt;
    LogLevel m_minLogLevel = LogLevel::Info;
    static constexpr int kMaxStoredLines = 5000;
    static constexpr int kMaxStoredErrors = 200;
};

} // namespace zarya
