#pragma once

#include "updater/AppUpdateAsset.h"
#include "updater/AppUpdateChannel.h"
#include "updater/AppUpdateManifest.h"

#include <QStringList>

namespace zarya {

class AppUpdatePolicy {
public:
    static QStringList buildWarnings(const AppUpdateChannelEntry& entry,
                                     const AppUpdateAsset& asset, bool allowUnsignedDownload);
    static QStringList buildBlockers(const AppUpdateChannelEntry& entry,
                                     const QString& currentVersion,
                                     AppUpdateChannel userChannel,
                                     const QString& selectedChannelKey,
                                     bool hasMatchingAsset);
};

} // namespace zarya
