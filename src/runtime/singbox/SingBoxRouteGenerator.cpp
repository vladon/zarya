#include "runtime/singbox/SingBoxRouteGenerator.h"

#include "routing/RoutingGeoUtils.h"
#include "runtime/singbox/SingBoxRuleSetManager.h"

#include <QJsonArray>
#include <algorithm>

namespace zarya {

namespace {

void appendWarning(QStringList* warnings, const QString& message)
{
    if (warnings) {
        warnings->append(message);
    }
}

QString outboundForAction(RoutingAction action)
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

struct RouteMatchers {
    QJsonArray geosite;
    QJsonArray geoip;
    QJsonArray domain;
    QJsonArray domainSuffix;
    QJsonArray domainKeyword;
    QJsonArray domainRegex;
    QJsonArray ipCidr;
    QJsonArray port;
    QJsonArray protocol;
    bool ipIsPrivate = false;
};

bool appendUnique(QJsonArray& array, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    for (const QJsonValue& existing : array) {
        if (existing.toString().compare(trimmed, Qt::CaseInsensitive) == 0) {
            return false;
        }
    }
    array.append(trimmed);
    return true;
}

bool translateDomainValue(const QString& rawValue, RouteMatchers& matchers, QStringList* warnings)
{
    QString value = rawValue.trimmed();
    if (value.isEmpty()) {
        appendWarning(warnings, QStringLiteral("Empty domain routing value skipped."));
        return false;
    }

    if (value.startsWith(QStringLiteral("geosite:"), Qt::CaseInsensitive)) {
        appendUnique(matchers.geosite, value.mid(8));
        return true;
    }
    if (value.startsWith(QStringLiteral("domain:"), Qt::CaseInsensitive)) {
        appendUnique(matchers.domainSuffix, value.mid(7));
        return true;
    }
    if (value.startsWith(QStringLiteral("full:"), Qt::CaseInsensitive)) {
        appendUnique(matchers.domain, value.mid(5));
        return true;
    }
    if (value.startsWith(QStringLiteral("regexp:"), Qt::CaseInsensitive)) {
        appendUnique(matchers.domainRegex, value.mid(7));
        return true;
    }
    if (value.startsWith(QStringLiteral("keyword:"), Qt::CaseInsensitive)) {
        appendUnique(matchers.domainKeyword, value.mid(8));
        return true;
    }

    if (value.contains(QLatin1Char(':'))) {
        appendWarning(warnings,
                      QStringLiteral("Unsupported domain prefix in value: %1").arg(value));
        return false;
    }

    appendUnique(matchers.domainSuffix, value);
    return true;
}

bool translateIpValue(const QString& rawValue, RouteMatchers& matchers, QStringList* warnings)
{
    QString value = rawValue.trimmed();
    if (value.isEmpty()) {
        appendWarning(warnings, QStringLiteral("Empty IP routing value skipped."));
        return false;
    }

    if (value.startsWith(QStringLiteral("geoip:"), Qt::CaseInsensitive)) {
        const QString tag = value.mid(6).trimmed().toLower();
        if (tag == QStringLiteral("private")) {
            matchers.ipIsPrivate = true;
            return true;
        }
        appendUnique(matchers.geoip, tag);
        return true;
    }

    appendUnique(matchers.ipCidr, value);
    return true;
}

bool translatePortValue(const QString& rawValue, RouteMatchers& matchers, QStringList* warnings)
{
    const QString value = rawValue.trimmed();
    if (value.isEmpty()) {
        appendWarning(warnings, QStringLiteral("Empty port routing value skipped."));
        return false;
    }
    if (value.contains(QLatin1Char('-'))) {
        appendWarning(warnings,
                      QStringLiteral("Port ranges are not supported in TUN routing yet: %1")
                          .arg(value));
        return false;
    }
    bool ok = false;
    value.toUShort(&ok);
    if (!ok) {
        appendWarning(warnings, QStringLiteral("Invalid port value skipped: %1").arg(value));
        return false;
    }
    appendUnique(matchers.port, value);
    return true;
}

bool translateProtocolValue(const QString& rawValue, RouteMatchers& matchers, QStringList* warnings)
{
    const QString value = rawValue.trimmed().toLower();
    if (value.isEmpty()) {
        appendWarning(warnings, QStringLiteral("Empty protocol routing value skipped."));
        return false;
    }
    static const QStringList supported = {QStringLiteral("bittorrent"), QStringLiteral("quic"),
                                          QStringLiteral("stun"),       QStringLiteral("dns"),
                                          QStringLiteral("tls"),        QStringLiteral("http"),
                                          QStringLiteral("https")};
    if (!supported.contains(value)) {
        appendWarning(warnings,
                      QStringLiteral("Protocol rule may not be supported by sing-box: %1")
                          .arg(value));
    }
    appendUnique(matchers.protocol, value);
    return true;
}

QJsonObject matchersToRule(const RouteMatchers& matchers, const QString& outbound)
{
    QJsonObject rule;
    bool hasMatcher = false;

    if (!matchers.geosite.isEmpty()) {
        rule.insert(QStringLiteral("geosite"), matchers.geosite);
        hasMatcher = true;
    }
    if (!matchers.geoip.isEmpty()) {
        rule.insert(QStringLiteral("geoip"), matchers.geoip);
        hasMatcher = true;
    }
    if (matchers.ipIsPrivate) {
        rule.insert(QStringLiteral("ip_is_private"), true);
        hasMatcher = true;
    }
    if (!matchers.domain.isEmpty()) {
        rule.insert(QStringLiteral("domain"), matchers.domain);
        hasMatcher = true;
    }
    if (!matchers.domainSuffix.isEmpty()) {
        rule.insert(QStringLiteral("domain_suffix"), matchers.domainSuffix);
        hasMatcher = true;
    }
    if (!matchers.domainKeyword.isEmpty()) {
        rule.insert(QStringLiteral("domain_keyword"), matchers.domainKeyword);
        hasMatcher = true;
    }
    if (!matchers.domainRegex.isEmpty()) {
        rule.insert(QStringLiteral("domain_regex"), matchers.domainRegex);
        hasMatcher = true;
    }
    if (!matchers.ipCidr.isEmpty()) {
        rule.insert(QStringLiteral("ip_cidr"), matchers.ipCidr);
        hasMatcher = true;
    }
    if (!matchers.port.isEmpty()) {
        rule.insert(QStringLiteral("port"), matchers.port);
        hasMatcher = true;
    }
    if (!matchers.protocol.isEmpty()) {
        rule.insert(QStringLiteral("protocol"), matchers.protocol);
        hasMatcher = true;
    }

    if (!hasMatcher) {
        return {};
    }

    rule.insert(QStringLiteral("outbound"), outbound);
    return rule;
}

QJsonObject ruleFromRoutingRule(const RoutingRule& routingRule, QStringList* warnings)
{
    RouteMatchers matchers;
    const QString outbound = outboundForAction(routingRule.action);
    bool any = false;

    for (const QString& value : routingRule.values) {
        switch (routingRule.type) {
        case RoutingRuleType::Domain:
            any = translateDomainValue(value, matchers, warnings) || any;
            break;
        case RoutingRuleType::Ip:
            any = translateIpValue(value, matchers, warnings) || any;
            break;
        case RoutingRuleType::Port:
            any = translatePortValue(value, matchers, warnings) || any;
            break;
        case RoutingRuleType::Protocol:
            any = translateProtocolValue(value, matchers, warnings) || any;
            break;
        }
    }

    if (!any) {
        return {};
    }
    return matchersToRule(matchers, outbound);
}

} // namespace

QJsonObject SingBoxRouteGenerator::generateRoute(const RoutingProfile& routingProfile,
                                                 const SingBoxRouteGenerationOptions& options,
                                                 QStringList* warnings) const
{
    QJsonObject route;
    if (options.enableAutoDetectInterface) {
        route.insert(QStringLiteral("auto_detect_interface"), true);
    }

    QJsonArray rules;

    if (routingProfile.mode == RoutingMode::ProxyAll) {
        route.insert(QStringLiteral("rules"), rules);
        route.insert(QStringLiteral("final"), options.finalOutbound);
        return route;
    }

    QVector<RoutingRule> sortedRules = routingProfile.rules;
    std::stable_sort(sortedRules.begin(), sortedRules.end(),
                     [](const RoutingRule& a, const RoutingRule& b) {
                         return actionSortOrder(a.action) < actionSortOrder(b.action);
                     });

    for (const RoutingRule& routingRule : sortedRules) {
        if (!routingRule.enabled) {
            continue;
        }
        const QJsonObject singBoxRule = ruleFromRoutingRule(routingRule, warnings);
        if (!singBoxRule.isEmpty()) {
            rules.append(singBoxRule);
        }
    }

    const QStringList geoValues = RoutingGeoUtils::geoTagsUsed(routingProfile);
    QStringList geoTagsOnly;
    for (const QString& value : geoValues) {
        const QString trimmed = value.trimmed();
        if (trimmed.startsWith(QStringLiteral("geosite:"), Qt::CaseInsensitive)) {
            geoTagsOnly.append(trimmed.mid(8).trimmed());
        } else if (trimmed.startsWith(QStringLiteral("geoip:"), Qt::CaseInsensitive)) {
            const QString tag = trimmed.mid(6).trimmed();
            if (tag.compare(QStringLiteral("private"), Qt::CaseInsensitive) != 0) {
                geoTagsOnly.append(tag);
            }
        }
    }
    geoTagsOnly.removeDuplicates();
    if (options.enableRuleSets && !geoTagsOnly.isEmpty()) {
        SingBoxRuleSetManager::appendGeoCompatibilityWarnings(geoTagsOnly, warnings);
    }

    route.insert(QStringLiteral("rules"), rules);
    route.insert(QStringLiteral("final"), options.finalOutbound);
    return route;
}

} // namespace zarya
