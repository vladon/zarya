#include "recovery/StartupRecovery.h"

#include "helperclient/HelperProcessManager.h"
#include "platform/SystemProxyController.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QDir>
#include <QFile>

namespace zarya {

StartupRecoveryPlan StartupRecovery::detect()
{
    StartupRecoveryPlan plan;
    const AppSettings& settings = AppSettings::instance();

    plan.tunMayHaveBeenRunning = settings.shouldWarnUncleanTunShutdown();
    plan.killSwitchMarkerPresent = QFile::exists(AppPaths::killSwitchMarkerPath());
    plan.runtimeTempFilesPresent =
        QFile::exists(AppPaths::xrayConfigPath()) || QFile::exists(AppPaths::singBoxConfigPath())
        || QFile::exists(AppPaths::singBoxTunConfigPath());

    plan.systemProxyMayBeEnabled =
        settings.restoreProxyOnExit()
        && (plan.tunMayHaveBeenRunning || plan.runtimeTempFilesPresent);

    if (plan.tunMayHaveBeenRunning) {
        plan.detectedLines.append(QStringLiteral("TUN mode may have been running"));
    }
    if (plan.systemProxyMayBeEnabled) {
        plan.detectedLines.append(QStringLiteral("System proxy may still be enabled"));
    }
    if (plan.killSwitchMarkerPresent) {
        plan.detectedLines.append(QStringLiteral("Kill switch marker is present"));
    }
    if (plan.runtimeTempFilesPresent) {
        plan.detectedLines.append(QStringLiteral("Temporary runtime configs are present"));
    }

    plan.uncleanShutdown = !plan.detectedLines.isEmpty();
    plan.disableKillSwitch = false;
    return plan;
}

bool StartupRecovery::apply(const StartupRecoveryPlan& plan, QStringList* logLines,
                            QString* errorMessage)
{
    if (plan.restoreSystemProxy && plan.systemProxyMayBeEnabled) {
        SystemProxyController proxy;
        QString proxyError;
        if (proxy.restorePreviousProxy(SystemProxyRestoreMode::Automatic,
                                       [&](const QString& line) {
                                           if (logLines) {
                                               logLines->append(line);
                                           }
                                       },
                                       &proxyError)) {
            if (logLines) {
                logLines->append(QStringLiteral("System proxy restored during recovery"));
            }
        } else if (!proxyError.isEmpty() && logLines) {
            logLines->append(QStringLiteral("System proxy restore: %1").arg(proxyError));
        }
    }

    if (plan.cleanRuntimeTempFiles && plan.runtimeTempFilesPresent) {
        for (const QString& path :
             {AppPaths::xrayConfigPath(), AppPaths::singBoxConfigPath(),
              AppPaths::singBoxTunConfigPath()}) {
            if (QFile::exists(path)) {
                QFile::remove(path);
            }
        }
        if (logLines) {
            logLines->append(QStringLiteral("Runtime temp configs cleaned"));
        }
    }

    if (plan.uncleanShutdown) {
        AppSettings::instance().markCleanShutdown();
    }

    Q_UNUSED(errorMessage);
    return true;
}

} // namespace zarya
