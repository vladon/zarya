#include "recovery/StartupRecovery.h"

#include "killswitch/KillSwitchManager.h"
#include "platform/ManagedCoreOrphanCleanup.h"
#include "platform/PlatformPrivilege.h"
#include "platform/SystemProxyController.h"
#include "platform/SystemProxyStateStore.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

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
    plan.orphanedManagedCoresPresent = hasOrphanedManagedCores();

    plan.systemProxyMayBeEnabled =
        settings.restoreProxyOnExit()
        && (plan.tunMayHaveBeenRunning || plan.runtimeTempFilesPresent
            || SystemProxyStateStore::exists());

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
    if (plan.orphanedManagedCoresPresent) {
        plan.detectedLines.append(QStringLiteral("Leftover managed core process is running"));
    }

    plan.uncleanShutdown = !plan.detectedLines.isEmpty();
    plan.disableKillSwitch = plan.killSwitchMarkerPresent;
    plan.stopOrphanedManagedCores = plan.orphanedManagedCoresPresent;
    return plan;
}

int StartupRecovery::plannedStepCount(const StartupRecoveryPlan& plan)
{
    int count = 0;
    if (plan.stopOrphanedManagedCores && plan.orphanedManagedCoresPresent) {
        ++count;
    }
    if (plan.restoreSystemProxy && plan.systemProxyMayBeEnabled) {
        ++count;
    }
    if (plan.disableKillSwitch && plan.killSwitchMarkerPresent) {
        ++count;
    }
    if (plan.cleanRuntimeTempFiles && plan.runtimeTempFilesPresent) {
        ++count;
    }
    if (plan.uncleanShutdown) {
        ++count; // mark clean shutdown
    }
    return count;
}

bool StartupRecovery::apply(const StartupRecoveryPlan& plan, QStringList* logLines,
                            QString* errorMessage,
                            const StartupRecoveryProgressCallback& progress)
{
    const int totalSteps = plannedStepCount(plan);
    int currentStep = 0;
    const auto reportProgress = [&]() {
        if (progress) {
            progress(currentStep, totalSteps);
        }
        ++currentStep;
    };

    if (plan.stopOrphanedManagedCores && plan.orphanedManagedCoresPresent) {
        reportProgress();
        const ManagedCoreOrphanCleanupResult cleanup = terminateOrphanedManagedCores();
        if (logLines) {
            for (const QString& line : cleanup.details) {
                logLines->append(line);
            }
            if (cleanup.terminatedCount == 0 && cleanup.details.isEmpty()) {
                logLines->append(QStringLiteral("No leftover managed core processes found"));
            }
        }
    }

    if (plan.restoreSystemProxy && plan.systemProxyMayBeEnabled) {
        reportProgress();
        SystemProxyController proxy;
        QString proxyError;
        const auto logLine = [&](const QString& line) {
            if (logLines) {
                logLines->append(line);
            }
        };

        bool restored = proxy.restorePersistedPreviousProxy(logLine, &proxyError);
        if (!restored) {
            restored = proxy.tryClearZaryaOwnedProxy(AppSettings::instance().mixedPort(), logLine,
                                                     &proxyError);
        }
        if (restored) {
            logLine(QStringLiteral("System proxy restored during recovery"));
        } else if (!proxyError.isEmpty()) {
            logLine(QStringLiteral("System proxy restore: %1").arg(proxyError));
        }
    }

    if (plan.disableKillSwitch && plan.killSwitchMarkerPresent) {
        reportProgress();
        const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
        KillSwitchManager manager;
        manager.refreshStartupState(privileges.elevated);
        QString killSwitchError;
        if (manager.recover(true, &killSwitchError)) {
            if (logLines) {
                logLines->append(QStringLiteral("Kill switch recovered"));
            }
        } else if (!killSwitchError.isEmpty() && logLines) {
            logLines->append(QStringLiteral("Kill switch recovery: %1").arg(killSwitchError));
        }
    }

    if (plan.cleanRuntimeTempFiles && plan.runtimeTempFilesPresent) {
        reportProgress();
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
        reportProgress();
        AppSettings::instance().markCleanShutdown();
    }

    if (progress && totalSteps > 0) {
        progress(totalSteps, totalSteps);
    }

    Q_UNUSED(errorMessage);
    return true;
}

} // namespace zarya
