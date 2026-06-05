#include "rulesets/RuleSetKind.h"

namespace zarya {

QString ruleSetKindToString(RuleSetKind kind)
{
    switch (kind) {
    case RuleSetKind::GeoSite:
        return QStringLiteral("geosite");
    case RuleSetKind::GeoIp:
        return QStringLiteral("geoip");
    case RuleSetKind::Domain:
        return QStringLiteral("domain");
    case RuleSetKind::Ip:
        return QStringLiteral("ip");
    case RuleSetKind::Mixed:
        return QStringLiteral("mixed");
    }
    return QStringLiteral("unknown");
}

RuleSetKind ruleSetKindFromString(const QString& value)
{
    if (value == QStringLiteral("geosite")) {
        return RuleSetKind::GeoSite;
    }
    if (value == QStringLiteral("geoip")) {
        return RuleSetKind::GeoIp;
    }
    if (value == QStringLiteral("domain")) {
        return RuleSetKind::Domain;
    }
    if (value == QStringLiteral("ip")) {
        return RuleSetKind::Ip;
    }
    return RuleSetKind::Mixed;
}

QString ruleSetFormatToString(RuleSetFormat format)
{
    return format == RuleSetFormat::BinarySrs ? QStringLiteral("binary-srs")
                                              : QStringLiteral("source-json");
}

RuleSetFormat ruleSetFormatFromString(const QString& value)
{
    if (value == QStringLiteral("binary-srs")) {
        return RuleSetFormat::BinarySrs;
    }
    return RuleSetFormat::SourceJson;
}

} // namespace zarya
