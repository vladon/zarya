#include "routing/XrayRoutingGenerator.h"

#include <QJsonArray>
#include <algorithm>

namespace zarya {

namespace {

QString outboundTagForAction(RoutingAction action)
{
    switch (action) {
    case RoutingAction::Direct:
        return QStringLiteral("direct");
    case RoutingAction::Block:
        return QStringLiteral("block");
    case RoutingAction::Proxy:
        return QStringLiteral("proxy");
    }
    return QStringLiteral("proxy");
}

QString ruleFieldKey(RoutingRuleType type)
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

int actionSortOrder(RoutingAction action)
{
    switch (action) {
    case RoutingAction::Block:
        return 0;
    case RoutingAction::Direct:
        return 1;
    case RoutingAction::Proxy:
        return 2;
    }
    return 2;
}

QJsonObject makeFieldRule(const QString& fieldKey, const QStringList& values,
                          const QString& outboundTag)
{
    QJsonObject rule;
    rule.insert(QStringLiteral("type"), QStringLiteral("field"));
    QJsonArray array;
    for (const QString& value : values) {
        const QString trimmed = value.trimmed();
        if (!trimmed.isEmpty()) {
            array.append(trimmed);
        }
    }
    if (array.isEmpty()) {
        return {};
    }
    rule.insert(fieldKey, array);
    rule.insert(QStringLiteral("outboundTag"), outboundTag);
    return rule;
}

} // namespace

int XrayRoutingGenerator::enabledRuleCount(const RoutingProfile& profile) const
{
    int count = 0;
    for (const RoutingRule& rule : profile.rules) {
        if (rule.enabled && !rule.values.isEmpty()) {
            ++count;
        }
    }
    return count;
}

QJsonObject XrayRoutingGenerator::generate(const RoutingProfile& profile) const
{
    QVector<RoutingRule> sortedRules = profile.rules;
    std::stable_sort(sortedRules.begin(), sortedRules.end(),
                     [](const RoutingRule& a, const RoutingRule& b) {
                         return actionSortOrder(a.action) < actionSortOrder(b.action);
                     });

    QJsonArray rules;
    for (const RoutingRule& routingRule : sortedRules) {
        if (!routingRule.enabled) {
            continue;
        }
        const QString fieldKey = ruleFieldKey(routingRule.type);
        const QJsonObject xrayRule =
            makeFieldRule(fieldKey, routingRule.values, outboundTagForAction(routingRule.action));
        if (!xrayRule.isEmpty()) {
            rules.append(xrayRule);
        }
    }

    QJsonObject defaultRule;
    defaultRule.insert(QStringLiteral("type"), QStringLiteral("field"));
    defaultRule.insert(QStringLiteral("network"), QStringLiteral("tcp,udp"));
    defaultRule.insert(QStringLiteral("outboundTag"), QStringLiteral("proxy"));
    rules.append(defaultRule);

    QString domainStrategy = profile.domainStrategy.trimmed();
    if (!RoutingProfile::isValidDomainStrategy(domainStrategy)) {
        domainStrategy = QStringLiteral("AsIs");
    }

    QJsonObject routing;
    routing.insert(QStringLiteral("domainStrategy"), domainStrategy);
    routing.insert(QStringLiteral("rules"), rules);
    return routing;
}

} // namespace zarya
