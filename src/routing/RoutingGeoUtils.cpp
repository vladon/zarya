#include "routing/RoutingGeoUtils.h"

namespace zarya {

bool RoutingGeoUtils::valueReferencesGeoData(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    const QString lower = trimmed.toLower();
    return lower.startsWith(QStringLiteral("geoip:"))
           || lower.startsWith(QStringLiteral("geosite:"))
           || lower.startsWith(QStringLiteral("ext:geoip.dat:"))
           || lower.startsWith(QStringLiteral("ext:geosite.dat:"));
}

QStringList RoutingGeoUtils::geoTagsUsed(const RoutingProfile& profile)
{
    QStringList tags;
    for (const RoutingRule& rule : profile.rules) {
        if (!rule.enabled) {
            continue;
        }
        for (const QString& value : rule.values) {
            if (valueReferencesGeoData(value) && !tags.contains(value)) {
                tags.append(value);
            }
        }
    }
    return tags;
}

bool RoutingGeoUtils::profileUsesGeoData(const RoutingProfile& profile)
{
    return !geoTagsUsed(profile).isEmpty();
}

} // namespace zarya
