#pragma once

#include <QString>

namespace zarya {

class RuleSetNormalizer {
public:
    static QString normalizeGeoReference(const QString& reference);
    static QString geositeTagFromValue(const QString& geoValue);
    static QString geoipTagFromValue(const QString& geoValue);
    static bool isGeoSiteReference(const QString& reference);
    static bool isGeoIpReference(const QString& reference);
};

} // namespace zarya
