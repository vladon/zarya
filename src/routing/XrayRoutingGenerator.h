#pragma once

#include "domain/RoutingProfile.h"

#include <QJsonObject>

namespace zarya {

class XrayRoutingGenerator {
public:
    QJsonObject generate(const RoutingProfile& profile) const;
    int enabledRuleCount(const RoutingProfile& profile) const;
};

} // namespace zarya
