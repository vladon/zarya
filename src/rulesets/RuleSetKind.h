#pragma once

#include <QString>

namespace zarya {

enum class RuleSetKind {
    GeoSite,
    GeoIp,
    Domain,
    Ip,
    Mixed,
};

enum class RuleSetFormat {
    SourceJson,
    BinarySrs,
};

QString ruleSetKindToString(RuleSetKind kind);
RuleSetKind ruleSetKindFromString(const QString& value);

QString ruleSetFormatToString(RuleSetFormat format);
RuleSetFormat ruleSetFormatFromString(const QString& value);

} // namespace zarya
