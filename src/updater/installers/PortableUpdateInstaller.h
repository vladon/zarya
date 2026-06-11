#pragma once

#include "updater/AppUpdateAsset.h"
#include "updater/runner/UpdatePlanFile.h"

#include <QString>

namespace zarya {

class PortableUpdateInstaller {
public:
    static bool isImplemented();

    static QString statusMessage();

    static bool canInstallPortableUpdate(const AppUpdateAsset& asset, bool artifactVerified,
                                         bool stagingReady, bool runtimeRunning, bool testsRunning,
                                         bool killSwitchActive, QString* reason = nullptr);

    static bool isAppImageMode();

    static UpdatePlan buildUpdatePlan(const QString& currentVersion, const QString& targetVersion,
                                      const QString& stagingDir);

    static QString resolvedUpdaterPath();

    static bool launchUpdaterAndQuit(const UpdatePlan& plan, QString* errorMessage = nullptr);
};

} // namespace zarya
