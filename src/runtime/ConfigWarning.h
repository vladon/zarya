#pragma once

#include <QList>
#include <QString>

namespace zarya {

enum class ConfigWarningSeverity { Info, Warning, Blocking };

struct ConfigWarning {
    ConfigWarningSeverity severity = ConfigWarningSeverity::Warning;
    QString message;

    static ConfigWarning info(const QString& message);
    static ConfigWarning warning(const QString& message);
    static ConfigWarning blocking(const QString& message);
};

bool hasBlockingWarnings(const QList<ConfigWarning>& warnings);
QStringList warningMessages(const QList<ConfigWarning>& warnings, ConfigWarningSeverity minSeverity);

} // namespace zarya
