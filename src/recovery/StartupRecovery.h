#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

struct StartupRecoveryPlan {
    bool uncleanShutdown = false;
    bool systemProxyMayBeEnabled = false;
    bool tunMayHaveBeenRunning = false;
    bool killSwitchMarkerPresent = false;
    bool runtimeTempFilesPresent = false;

    bool restoreSystemProxy = true;
    bool stopHelperRuntime = true;
    bool disableKillSwitch = false;
    bool cleanRuntimeTempFiles = true;

    QStringList detectedLines;
};

class StartupRecovery {
public:
    static StartupRecoveryPlan detect();
    static bool apply(const StartupRecoveryPlan& plan, QStringList* logLines,
                      QString* errorMessage = nullptr);
};

} // namespace zarya
