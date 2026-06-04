#include "runtime/ConfigWarning.h"

namespace zarya {

ConfigWarning ConfigWarning::info(const QString& message)
{
    ConfigWarning warning;
    warning.severity = ConfigWarningSeverity::Info;
    warning.message = message;
    return warning;
}

ConfigWarning ConfigWarning::warning(const QString& message)
{
    ConfigWarning warning;
    warning.severity = ConfigWarningSeverity::Warning;
    warning.message = message;
    return warning;
}

ConfigWarning ConfigWarning::blocking(const QString& message)
{
    ConfigWarning warning;
    warning.severity = ConfigWarningSeverity::Blocking;
    warning.message = message;
    return warning;
}

bool hasBlockingWarnings(const QList<ConfigWarning>& warnings)
{
    for (const ConfigWarning& warning : warnings) {
        if (warning.severity == ConfigWarningSeverity::Blocking) {
            return true;
        }
    }
    return false;
}

QStringList warningMessages(const QList<ConfigWarning>& warnings,
                            ConfigWarningSeverity minSeverity)
{
    QStringList messages;
    for (const ConfigWarning& warning : warnings) {
        if (static_cast<int>(warning.severity) >= static_cast<int>(minSeverity)) {
            messages.append(warning.message);
        }
    }
    return messages;
}

} // namespace zarya
