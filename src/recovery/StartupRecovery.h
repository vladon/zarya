#pragma once

#include <QString>
#include <QStringList>

#include <functional>

namespace zarya {

struct StartupRecoveryPlan {
    bool uncleanShutdown = false;
    bool systemProxyMayBeEnabled = false;
    bool tunMayHaveBeenRunning = false;
    bool killSwitchMarkerPresent = false;
    bool runtimeTempFilesPresent = false;
    bool orphanedManagedCoresPresent = false;

    bool restoreSystemProxy = true;
    bool stopHelperRuntime = true;
    bool disableKillSwitch = false;
    bool cleanRuntimeTempFiles = true;
    bool stopOrphanedManagedCores = true;

    QStringList detectedLines;
};

/// Called before each recovery step with 0-based step index and total step count.
using StartupRecoveryProgressCallback = std::function<void(int currentStep, int totalSteps)>;

class StartupRecovery {
public:
    static StartupRecoveryPlan detect();
    static int plannedStepCount(const StartupRecoveryPlan& plan);
    static bool apply(const StartupRecoveryPlan& plan, QStringList* logLines,
                      QString* errorMessage = nullptr,
                      const StartupRecoveryProgressCallback& progress = {});
};

} // namespace zarya
