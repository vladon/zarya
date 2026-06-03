#include "geodata/GeoDataSource.h"

namespace zarya {

namespace {

GeoDataSource makeLoyalsoldier()
{
    GeoDataSource source;
    source.id = QStringLiteral("loyalsoldier");
    source.name = QStringLiteral("Loyalsoldier v2ray-rules-dat");
    source.description =
        QStringLiteral("Enhanced routing data compatible with Xray/V2Ray-style geoip/geosite tags.");
    source.geoipUrl = QUrl(
        QStringLiteral("https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/"
                         "geoip.dat"));
    source.geoipSha256Url = QUrl(
        QStringLiteral("https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/"
                         "geoip.dat.sha256sum"));
    source.geositeUrl = QUrl(
        QStringLiteral("https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/"
                         "geosite.dat"));
    source.geositeSha256Url = QUrl(
        QStringLiteral(
            "https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/"
            "geosite.dat.sha256sum"));
    source.enabled = true;
    source.builtIn = true;
    return source;
}

} // namespace

QVector<GeoDataSource> GeoDataSources::builtInSources()
{
    return {makeLoyalsoldier()};
}

GeoDataSource GeoDataSources::sourceById(const QString& id)
{
    for (const GeoDataSource& source : builtInSources()) {
        if (source.id == id) {
            return source;
        }
    }
    return makeLoyalsoldier();
}

} // namespace zarya
