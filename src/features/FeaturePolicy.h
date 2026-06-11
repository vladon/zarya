#pragma once

#include "features/FeatureId.h"

#include <QString>

namespace zarya {

enum class ReleaseChannel {
    Dev,
    Beta,
    Rc,
    Stable,
};

class FeaturePolicy {
public:
    static ReleaseChannel releaseChannelFromString(const QString& text);
    static QString releaseChannelToString(ReleaseChannel channel);
    static ReleaseChannel defaultReleaseChannelFromBuild();
    static bool defaultShowExperimentalFeatures(ReleaseChannel channel);
    static bool isStableLikeChannel(ReleaseChannel channel);
    static bool isExperimentalFeature(FeatureId id);
};

} // namespace zarya
