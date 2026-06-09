#include "geodata/GeoDataFileStatus.h"

#include "i18n/TranslatableEnums.h"

namespace zarya {

QString geoDataKindFileName(GeoDataKind kind)
{
    switch (kind) {
    case GeoDataKind::GeoSite:
        return QStringLiteral("geosite.dat");
    case GeoDataKind::GeoIp:
        return QStringLiteral("geoip.dat");
    }
    return QStringLiteral("geoip.dat");
}

QString geoDataStatusDisplayString(GeoDataStatus status)
{
    return TranslatableEnums::trGeoDataStatus(status);
}

} // namespace zarya
