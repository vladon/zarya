#pragma once

#include "updater/AppUpdateAsset.h"
#include "updater/AppUpdateChannel.h"
#include "updater/AppUpdateManifest.h"

#include <QStringList>

namespace zarya {

struct AppUpdatePlan {
    bool updateAvailable = false;
    QString currentVersion;
    QString latestVersion;
    QString selectedChannelKey;
    AppUpdateChannelEntry channelEntry;
    AppUpdateAsset selectedAsset;
    QStringList warnings;
    QStringList blockers;
};

class AppUpdatePlanner {
public:
    static AppUpdatePlan buildPlan(const AppUpdateManifest& manifest, const QString& currentVersion,
                                   AppUpdateChannel userChannel);
};

} // namespace zarya
