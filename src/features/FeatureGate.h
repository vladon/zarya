#pragma once

#include "features/FeatureId.h"

#include <QJsonObject>
#include <QString>

namespace zarya {

class FeatureGate {
public:
    static bool showExperimentalFeatures();
    static QString releaseChannelKey();
    static bool isVisible(FeatureId id);
    static bool isEnabled(FeatureId id);
    static bool experimentalRuntimeConfigured();
    static bool experimentalRuntimeEffective();
    static QJsonObject diagnosticsJson();
};

} // namespace zarya
