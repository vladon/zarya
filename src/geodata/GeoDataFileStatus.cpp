#include "geodata/GeoDataFileStatus.h"

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
    switch (status) {
    case GeoDataStatus::Missing:
        return QStringLiteral("Missing");
    case GeoDataStatus::Present:
        return QStringLiteral("Present");
    case GeoDataStatus::Updating:
        return QStringLiteral("Updating");
    case GeoDataStatus::Verified:
        return QStringLiteral("Verified");
    case GeoDataStatus::ChecksumFailed:
        return QStringLiteral("Checksum failed");
    case GeoDataStatus::DownloadFailed:
        return QStringLiteral("Download failed");
    case GeoDataStatus::NotWritable:
        return QStringLiteral("Not writable");
    case GeoDataStatus::Unknown:
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

} // namespace zarya
