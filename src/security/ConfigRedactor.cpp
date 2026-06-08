#include "security/ConfigRedactor.h"

#include "diagnostics/DiagnosticsRedactor.h"
#include "backup/BackupExportOptions.h"

namespace zarya {

namespace {

DiagnosticsRedactionMode toDiagnosticsMode(RedactionStrength strength)
{
    return strength == RedactionStrength::Strict ? DiagnosticsRedactionMode::Strict
                                                 : DiagnosticsRedactionMode::Basic;
}

BackupRedactionMode toBackupMode(RedactionStrength strength)
{
    return strength == RedactionStrength::Strict ? BackupRedactionMode::Strict
                                                 : BackupRedactionMode::Basic;
}

} // namespace

QString ConfigRedactor::redactText(const QString& text, RedactionStrength strength)
{
    return DiagnosticsRedactor::redactText(text, toDiagnosticsMode(strength));
}

QString ConfigRedactor::redactLogLine(const QString& line, RedactionStrength strength)
{
    return DiagnosticsRedactor::redactLogLine(line, toDiagnosticsMode(strength));
}

QJsonObject ConfigRedactor::redactJsonObject(const QJsonObject& object, RedactionStrength strength)
{
    return DiagnosticsRedactor::redactJsonObject(object, toDiagnosticsMode(strength));
}

QString ConfigRedactor::redactPath(const QString& path, RedactionStrength strength,
                                     bool includeMachinePaths)
{
    return DiagnosticsRedactor::redactPath(path, toDiagnosticsMode(strength), includeMachinePaths);
}

} // namespace zarya
