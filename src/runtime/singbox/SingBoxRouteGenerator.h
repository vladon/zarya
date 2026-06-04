#pragma once

#include "domain/RoutingProfile.h"

#include <QJsonObject>
#include <QStringList>

namespace zarya {

struct SingBoxRouteGenerationOptions {
    bool tunMode = true;
    bool enableAutoDetectInterface = true;
    bool enableRuleSets = true;
    QString finalOutbound = QStringLiteral("proxy");
};

class SingBoxRouteGenerator {
public:
    QJsonObject generateRoute(const RoutingProfile& routingProfile,
                              const SingBoxRouteGenerationOptions& options,
                              QStringList* warnings = nullptr) const;
};

} // namespace zarya
