#include "domain/RoutingRule.h"

#include <QUuid>

namespace zarya {

RoutingRule RoutingRule::createDefault()
{
    RoutingRule rule;
    rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rule.enabled = true;
    rule.type = RoutingRuleType::Domain;
    rule.action = RoutingAction::Proxy;
    return rule;
}

} // namespace zarya
