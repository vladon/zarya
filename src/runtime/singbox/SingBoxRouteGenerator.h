#pragma once

#include "domain/RoutingProfile.h"
#include "runtime/singbox/SingBoxRuleSetContext.h"

#include <QJsonObject>
#include <QSet>
#include <QStringList>

namespace zarya {

struct SingBoxRouteGenerationOptions {
    bool tunMode = true;
    bool enableAutoDetectInterface = true;
    bool enableRuleSets = true;
    QString finalOutbound = QStringLiteral("proxy");
    SingBoxRuleSetContext ruleSetContext;
    QSet<QString>* usedRuleSetTagsOut = nullptr;
};

class SingBoxRouteGenerator {
public:
    QJsonObject generateRoute(const RoutingProfile& routingProfile,
                              const SingBoxRouteGenerationOptions& options,
                              QStringList* warnings = nullptr) const;
};

} // namespace zarya
