#pragma once

#include <QString>

namespace zarya {

class UpdaterLog {
public:
    explicit UpdaterLog(const QString& logFilePath);

    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

    QString logFilePath() const;

private:
    void writeLine(const QString& level, const QString& message);

    QString m_logFilePath;
};

} // namespace zarya
