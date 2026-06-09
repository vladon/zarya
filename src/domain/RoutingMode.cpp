#include "domain/RoutingMode.h"

#include "i18n/TranslatableEnums.h"

namespace zarya {

QString routingModeToString(RoutingMode mode)
{
    switch (mode) {
    case RoutingMode::ProxyAll:
        return QStringLiteral("proxy_all");
    case RoutingMode::BypassLan:
        return QStringLiteral("bypass_lan");
    case RoutingMode::BypassRu:
        return QStringLiteral("bypass_ru");
    case RoutingMode::BypassLanAndRu:
        return QStringLiteral("bypass_lan_and_ru");
    case RoutingMode::Custom:
        return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

RoutingMode routingModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("proxy_all")) {
        return RoutingMode::ProxyAll;
    }
    if (normalized == QStringLiteral("bypass_lan")) {
        return RoutingMode::BypassLan;
    }
    if (normalized == QStringLiteral("bypass_ru")) {
        return RoutingMode::BypassRu;
    }
    if (normalized == QStringLiteral("bypass_lan_and_ru")) {
        return RoutingMode::BypassLanAndRu;
    }
    return RoutingMode::Custom;
}

QString routingModeDisplayString(RoutingMode mode)
{
    return TranslatableEnums::trRoutingMode(mode);
}

QString routingActionToString(RoutingAction action)
{
    switch (action) {
    case RoutingAction::Proxy:
        return QStringLiteral("proxy");
    case RoutingAction::Direct:
        return QStringLiteral("direct");
    case RoutingAction::Block:
        return QStringLiteral("block");
    }
    return QStringLiteral("proxy");
}

RoutingAction routingActionFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("direct")) {
        return RoutingAction::Direct;
    }
    if (normalized == QStringLiteral("block")) {
        return RoutingAction::Block;
    }
    return RoutingAction::Proxy;
}

QString routingActionDisplayString(RoutingAction action)
{
    return TranslatableEnums::trRoutingAction(action);
}

QString routingRuleTypeToString(RoutingRuleType type)
{
    switch (type) {
    case RoutingRuleType::Domain:
        return QStringLiteral("domain");
    case RoutingRuleType::Ip:
        return QStringLiteral("ip");
    case RoutingRuleType::Port:
        return QStringLiteral("port");
    case RoutingRuleType::Protocol:
        return QStringLiteral("protocol");
    }
    return QStringLiteral("domain");
}

RoutingRuleType routingRuleTypeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("ip")) {
        return RoutingRuleType::Ip;
    }
    if (normalized == QStringLiteral("port")) {
        return RoutingRuleType::Port;
    }
    if (normalized == QStringLiteral("protocol")) {
        return RoutingRuleType::Protocol;
    }
    return RoutingRuleType::Domain;
}

QString routingRuleTypeDisplayString(RoutingRuleType type)
{
    switch (type) {
    case RoutingRuleType::Domain:
        return QStringLiteral("Domain");
    case RoutingRuleType::Ip:
        return QStringLiteral("IP");
    case RoutingRuleType::Port:
        return QStringLiteral("Port");
    case RoutingRuleType::Protocol:
        return QStringLiteral("Protocol");
    }
    return QStringLiteral("Domain");
}

namespace RoutingBuiltinIds {

QString proxyAll()
{
    return QStringLiteral("builtin-proxy-all");
}

QString bypassLan()
{
    return QStringLiteral("builtin-bypass-lan");
}

QString bypassRu()
{
    return QStringLiteral("builtin-bypass-ru");
}

QString bypassLanAndRu()
{
    return QStringLiteral("builtin-bypass-lan-ru");
}

QString customTemplate()
{
    return QStringLiteral("builtin-custom-template");
}

} // namespace RoutingBuiltinIds

} // namespace zarya
