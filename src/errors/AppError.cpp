#include "errors/AppError.h"

#include "errors/ErrorCode.h"
#include "i18n/TranslatableEnums.h"
#include "i18n/ZaryaTr.h"

namespace zarya {

QString errorSeverityToString(ErrorSeverity severity)
{
    return TranslatableEnums::trErrorSeverity(severity);
}

AppError appErrorFromCode(const QString& code, const QString& details)
{
    AppError error;
    error.code = code;
    error.details = details;

    if (code == ErrorCode::coreXrayMissing()) {
        error.title = ZaryaTr::tr("Xray not found");
        error.message = ZaryaTr::tr("The Xray executable is missing or not configured.");
        error.suggestedAction = ZaryaTr::tr("Open Core Manager and install Xray.");
        error.area = QStringLiteral("Core");
    } else if (code == ErrorCode::helperNotConnected()) {
        error.title = ZaryaTr::tr("Helper not connected");
        error.message = ZaryaTr::tr("zarya-helper is not running or not reachable.");
        error.suggestedAction = ZaryaTr::tr("Start the helper process and retry.");
        error.area = QStringLiteral("Helper");
    } else if (code == ErrorCode::killSwitchNeedsRecovery()) {
        error.title = ZaryaTr::tr("Kill switch recovery required");
        error.message = ZaryaTr::tr("A kill switch recovery marker is present.");
        error.suggestedAction = ZaryaTr::tr("Disable kill switch or follow recovery steps.");
        error.severity = ErrorSeverity::Critical;
        error.area = QStringLiteral("KillSwitch");
    } else if (code == ErrorCode::migrationFailed()) {
        error.title = ZaryaTr::tr("Configuration migration failed");
        error.message = ZaryaTr::tr("Zarya could not migrate local data files safely.");
        error.suggestedAction = ZaryaTr::tr("Check .bak files and logs; restore from backup.");
        error.severity = ErrorSeverity::Critical;
        error.area = QStringLiteral("Migration");
    } else if (code == ErrorCode::profileUnsupportedRuntime()) {
        error.title = ZaryaTr::tr("Unsupported profile");
        error.message = ZaryaTr::tr("This profile is not supported by the current runtime.");
        error.suggestedAction = ZaryaTr::tr("Switch to Xray system proxy or edit the profile.");
        error.area = QStringLiteral("Runtime");
    } else if (code == ErrorCode::backupImportChecksumFailed()) {
        error.title = ZaryaTr::tr("Backup checksum failed");
        error.message = ZaryaTr::tr("The backup archive failed checksum verification.");
        error.suggestedAction = ZaryaTr::tr("Re-export backup or verify archive integrity.");
        error.area = QStringLiteral("Backup");
    } else {
        error.title = ZaryaTr::tr("Error");
        error.message = details.isEmpty() ? ZaryaTr::tr("An error occurred.") : details;
        error.area = QStringLiteral("General");
    }
    return error;
}

} // namespace zarya
