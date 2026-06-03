#pragma once

#include "domain/RoutingMode.h"

#include <QString>
#include <QStringList>

namespace zarya {

struct RoutingRule {
    QString id;
    bool enabled = true;
    RoutingRuleType type = RoutingRuleType::Domain;
    RoutingAction action = RoutingAction::Proxy;
    QStringList values;
    QString note;

    static RoutingRule createDefault();
};

} // namespace zarya
