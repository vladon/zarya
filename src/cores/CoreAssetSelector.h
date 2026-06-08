#pragma once

#include "cores/CoreAsset.h"
#include "cores/CorePlatform.h"
#include "cores/CoreRelease.h"
#include "domain/CoreType.h"

#include <optional>

namespace zarya {

class CoreAssetSelector {
public:
    static std::optional<CoreAsset> selectBest(const CoreRelease& release,
                                               const CorePlatformInfo& platform);

private:
    static int scoreAsset(const CoreAsset& asset, CoreType type,
                          const CorePlatformInfo& platform);
    static bool isRejectedAsset(const QString& name);
};

} // namespace zarya
