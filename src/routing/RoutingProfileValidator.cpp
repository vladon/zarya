#include "routing/RoutingProfileValidator.h"

#include <QRegularExpression>

namespace zarya {

namespace {

bool looksLikeDomainValue(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    static const QStringList prefixes = {
        QStringLiteral("geosite:"), QStringLiteral("domain:"), QStringLiteral("full:"),
        QStringLiteral("regexp:"), QStringLiteral("keyword:"),
    };
    for (const QString& prefix : prefixes) {
        if (trimmed.startsWith(prefix, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return trimmed.contains(QLatin1Char('.'));
}

bool looksLikeIpValue(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.startsWith(QStringLiteral("geoip:"), Qt::CaseInsensitive)) {
        return true;
    }
    if (trimmed.contains(QLatin1Char('/'))) {
        return true;
    }
    static const QRegularExpression ipv4Pattern(
        QStringLiteral("^(?:\\d{1,3}\\.){3}\\d{1,3}$"));
    return ipv4Pattern.match(trimmed).hasMatch();
}

bool looksLikePortValue(const QString& value)
{
    static const QRegularExpression pattern(QStringLiteral("^\\d{1,5}(-\\d{1,5})?$"));
    return pattern.match(value.trimmed()).hasMatch();
}

} // namespace

QStringList RoutingProfileValidator::warnings(const RoutingProfile& profile)
{
    QStringList result;

    if (!RoutingProfile::isValidDomainStrategy(profile.domainStrategy)) {
        result.append(QStringLiteral("Domain strategy must be AsIs, IPIfNonMatch, or IPOnDemand."));
    }

    for (const RoutingRule& rule : profile.rules) {
        if (!rule.enabled) {
            continue;
        }
        for (const QString& value : rule.values) {
            const QString trimmed = value.trimmed();
            if (trimmed.isEmpty()) {
                continue;
            }
            switch (rule.type) {
            case RoutingRuleType::Domain:
                if (!looksLikeDomainValue(trimmed)) {
                    result.append(QStringLiteral("Domain rule value looks unusual: %1").arg(trimmed));
                }
                break;
            case RoutingRuleType::Ip:
                if (!looksLikeIpValue(trimmed)) {
                    result.append(QStringLiteral("IP rule value looks unusual: %1").arg(trimmed));
                }
                break;
            case RoutingRuleType::Port:
                if (!looksLikePortValue(trimmed)) {
                    result.append(QStringLiteral("Port rule value looks unusual: %1").arg(trimmed));
                }
                break;
            case RoutingRuleType::Protocol:
                break;
            }
        }
    }

    return result;
}

} // namespace zarya
