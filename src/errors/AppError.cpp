#include "errors/AppError.h"

#include "errors/ErrorCode.h"

namespace zarya {

QString errorSeverityToString(ErrorSeverity severity)
{
    switch (severity) {
    case ErrorSeverity::Info:
        return QStringLiteral("Info");
    case ErrorSeverity::Warning:
        return QStringLiteral("Warning");
    case ErrorSeverity::Error:
        return QStringLiteral("Error");
    case ErrorSeverity::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Error");
}

AppError appErrorFromCode(const QString& code, const QString& details)
{
    AppError error;
    error.code = code;
    error.details = details;

    if (code == ErrorCode::coreXrayMissing()) {
        error.title = QStringLiteral("Xray not found");
        error.message = QStringLiteral("The Xray executable is missing or not configured.");
        error.suggestedAction = QStringLiteral("Open Core Manager and install Xray.");
        error.area = QStringLiteral("Core");
    } else if (code == ErrorCode::helperNotConnected()) {
        error.title = QStringLiteral("Helper not connected");
        error.message = QStringLiteral("zarya-helper is not running or not reachable.");
        error.suggestedAction = QStringLiteral("Start the helper process and retry.");
        error.area = QStringLiteral("Helper");
    } else if (code == ErrorCode::killSwitchNeedsRecovery()) {
        error.title = QStringLiteral("Kill switch recovery required");
        error.message = QStringLiteral("A kill switch recovery marker is present.");
        error.suggestedAction = QStringLiteral("Disable kill switch or follow recovery steps.");
        error.severity = ErrorSeverity::Critical;
        error.area = QStringLiteral("KillSwitch");
    } else if (code == ErrorCode::migrationFailed()) {
        error.title = QStringLiteral("Configuration migration failed");
        error.message = QStringLiteral("Zarya could not migrate local data files safely.");
        error.suggestedAction = QStringLiteral("Check .bak files and logs; restore from backup.");
        error.severity = ErrorSeverity::Critical;
        error.area = QStringLiteral("Migration");
    } else if (code == ErrorCode::backupImportChecksumFailed()) {
        error.title = QStringLiteral("Backup checksum failed");
        error.message = QStringLiteral("The backup archive failed checksum verification.");
        error.suggestedAction = QStringLiteral("Re-export backup or verify archive integrity.");
        error.area = QStringLiteral("Backup");
    } else {
        error.title = QStringLiteral("Error");
        error.message = details.isEmpty() ? QStringLiteral("An error occurred.") : details;
        error.area = QStringLiteral("General");
    }
    return error;
}

} // namespace zarya
