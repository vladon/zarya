#pragma once

#include "domain/RoutingProfile.h"

#include <QStringList>

namespace zarya {

class RoutingGeoUtils {
public:
    static bool profileUsesGeoData(const RoutingProfile& profile);
    static QStringList geoTagsUsed(const RoutingProfile& profile);
    static bool valueReferencesGeoData(const QString& value);
};

} // namespace zarya
