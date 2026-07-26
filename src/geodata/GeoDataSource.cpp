#include "geodata/GeoDataSource.h"

namespace zarya {

namespace {

GeoDataSource makeReleaseSource(const QString& id, const QString& name, const QString& description,
                                const QString& repoPath)
{
    GeoDataSource source;
    source.id = id;
    source.name = name;
    source.description = description;
    const QString base =
        QStringLiteral("https://github.com/%1/releases/latest/download/").arg(repoPath);
    source.geoipUrl = QUrl(base + QStringLiteral("geoip.dat"));
    source.geoipSha256Url = QUrl(base + QStringLiteral("geoip.dat.sha256sum"));
    source.geositeUrl = QUrl(base + QStringLiteral("geosite.dat"));
    source.geositeSha256Url = QUrl(base + QStringLiteral("geosite.dat.sha256sum"));
    source.enabled = true;
    source.builtIn = true;
    return source;
}

GeoDataSource makeLoyalsoldier()
{
    return makeReleaseSource(
        QStringLiteral("loyalsoldier"), QStringLiteral("Loyalsoldier v2ray-rules-dat"),
        QStringLiteral(
            "General-purpose geoip/geosite lists compatible with Xray/V2Ray-style routing tags."),
        QStringLiteral("Loyalsoldier/v2ray-rules-dat"));
}

GeoDataSource makeRunetfreedom()
{
    return makeReleaseSource(
        QStringLiteral("runetfreedom"),
        QStringLiteral("runetfreedom russia-v2ray-rules-dat"),
        QStringLiteral(
            "Russia-focused lists (blocked resources, ru-blocked tags, antifilter categories)."),
        QStringLiteral("runetfreedom/russia-v2ray-rules-dat"));
}

GeoDataSource makeChocolate4u()
{
    return makeReleaseSource(
        QStringLiteral("chocolate4u"), QStringLiteral("Chocolate4U Iran-v2ray-rules"),
        QStringLiteral(
            "Iran-focused lists (geosite:ir / geoip:ir, ads and malware categories, and more)."),
        QStringLiteral("Chocolate4U/Iran-v2ray-rules"));
}

} // namespace

QVector<GeoDataSource> GeoDataSources::builtInSources()
{
    return {makeLoyalsoldier(), makeRunetfreedom(), makeChocolate4u()};
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
