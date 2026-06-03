#pragma once

#include <QString>

namespace zarya {

enum class RoutingMode {
    ProxyAll,
    BypassLan,
    BypassRu,
    BypassLanAndRu,
    Custom,
};

enum class RoutingAction {
    Proxy,
    Direct,
    Block,
};

enum class RoutingRuleType {
    Domain,
    Ip,
    Port,
    Protocol,
};

QString routingModeToString(RoutingMode mode);
RoutingMode routingModeFromString(const QString& value);
QString routingModeDisplayString(RoutingMode mode);

QString routingActionToString(RoutingAction action);
RoutingAction routingActionFromString(const QString& value);
QString routingActionDisplayString(RoutingAction action);

QString routingRuleTypeToString(RoutingRuleType type);
RoutingRuleType routingRuleTypeFromString(const QString& value);
QString routingRuleTypeDisplayString(RoutingRuleType type);

namespace RoutingBuiltinIds {
QString proxyAll();
QString bypassLan();
QString bypassRu();
QString bypassLanAndRu();
QString customTemplate();
} // namespace RoutingBuiltinIds

} // namespace zarya
