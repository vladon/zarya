#include "domain/RoutingProfile.h"

#include <QUuid>

namespace zarya {

namespace {

RoutingRule makeRule(RoutingRuleType type, RoutingAction action, const QStringList& values,
                     const QString& note = {})
{
    RoutingRule rule;
    rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rule.enabled = true;
    rule.type = type;
    rule.action = action;
    rule.values = values;
    rule.note = note;
    return rule;
}

RoutingProfile makeBuiltIn(const QString& id, const QString& name, RoutingMode mode,
                           const QVector<RoutingRule>& rules)
{
    RoutingProfile profile;
    profile.id = id;
    profile.name = name;
    profile.mode = mode;
    profile.enabled = true;
    profile.domainStrategy = QStringLiteral("AsIs");
    profile.rules = rules;
    profile.isBuiltIn = true;
    profile.createdAt = QDateTime::currentDateTimeUtc();
    profile.updatedAt = profile.createdAt;
    return profile;
}

} // namespace

bool RoutingProfile::isValidDomainStrategy(const QString& strategy)
{
    const QString normalized = strategy.trimmed();
    return normalized == QStringLiteral("AsIs") || normalized == QStringLiteral("IPIfNonMatch")
           || normalized == QStringLiteral("IPOnDemand");
}

RoutingProfile RoutingProfile::builtInProxyAll()
{
    return makeBuiltIn(RoutingBuiltinIds::proxyAll(), QStringLiteral("Proxy All"),
                       RoutingMode::ProxyAll, {});
}

QVector<RoutingProfile> RoutingProfile::createBuiltInProfiles()
{
    QVector<RoutingProfile> profiles;
    profiles.append(builtInProxyAll());
    profiles.append(makeBuiltIn(
        RoutingBuiltinIds::bypassLan(), QStringLiteral("Bypass LAN"), RoutingMode::BypassLan,
        {makeRule(RoutingRuleType::Domain, RoutingAction::Direct, {QStringLiteral("geosite:private")}),
         makeRule(RoutingRuleType::Ip, RoutingAction::Direct, {QStringLiteral("geoip:private")})}));
    profiles.append(makeBuiltIn(
        RoutingBuiltinIds::bypassRu(), QStringLiteral("Bypass RU"), RoutingMode::BypassRu,
        {makeRule(RoutingRuleType::Domain, RoutingAction::Direct, {QStringLiteral("geosite:ru")}),
         makeRule(RoutingRuleType::Ip, RoutingAction::Direct, {QStringLiteral("geoip:ru")})}));
    profiles.append(makeBuiltIn(
        RoutingBuiltinIds::bypassLanAndRu(), QStringLiteral("Bypass LAN + RU"),
        RoutingMode::BypassLanAndRu,
        {makeRule(RoutingRuleType::Domain, RoutingAction::Direct,
                  {QStringLiteral("geosite:private"), QStringLiteral("geosite:ru")}),
         makeRule(RoutingRuleType::Ip, RoutingAction::Direct,
                  {QStringLiteral("geoip:private"), QStringLiteral("geoip:ru")})}));
    profiles.append(makeBuiltIn(RoutingBuiltinIds::customTemplate(), QStringLiteral("Custom"),
                                RoutingMode::Custom, {}));
    return profiles;
}

} // namespace zarya
