#pragma once

#include <QDateTime>
#include <QString>

namespace zarya {

enum class GeoDataKind {
    GeoIp,
    GeoSite,
};

enum class GeoDataStatus {
    Missing,
    Present,
    Updating,
    Verified,
    ChecksumFailed,
    DownloadFailed,
    NotWritable,
    Unknown,
};

struct GeoDataFileStatus {
    GeoDataKind kind = GeoDataKind::GeoIp;
    QString fileName;
    QString path;
    GeoDataStatus status = GeoDataStatus::Unknown;
    qint64 sizeBytes = 0;
    QDateTime modifiedAt;
    QString sha256;
    QString expectedSha256;
    QString error;
};

QString geoDataKindFileName(GeoDataKind kind);
QString geoDataStatusDisplayString(GeoDataStatus status);

} // namespace zarya
