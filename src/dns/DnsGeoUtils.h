#pragma once

#include "domain/DnsProfile.h"

#include <QStringList>

namespace zarya {

class DnsGeoUtils {
public:
    static bool valueReferencesGeoData(const QString& value);
    static QStringList geoReferencesUsed(const DnsProfile& profile);
    static bool profileUsesGeoData(const DnsProfile& profile);
};

} // namespace zarya
