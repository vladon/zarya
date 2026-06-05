#include "rulesets/RuleSetCatalog.h"

namespace zarya {

namespace {

RuleSetItem makeBuiltIn(const QString& tag, const QString& name, RuleSetKind kind,
                        const QString& description)
{
    RuleSetItem item;
    item.id = tag;
    item.tag = tag;
    item.name = name;
    item.kind = kind;
    item.builtIn = true;
    item.enabled = true;
    item.description = description;
    item.status = RuleSetStatus::SourceMissing;
    return item;
}

} // namespace

QVector<RuleSetItem> RuleSetCatalog::builtInItems()
{
    return {
        makeBuiltIn(QStringLiteral("geosite-private"), QStringLiteral("GeoSite Private"),
                    RuleSetKind::GeoSite,
                    QStringLiteral("Built-in category; install .srs manually or add a custom URL.")),
        makeBuiltIn(QStringLiteral("geoip-private"), QStringLiteral("GeoIP Private"), RuleSetKind::GeoIp,
                    QStringLiteral("Built-in category; install .srs manually or add a custom URL.")),
        makeBuiltIn(QStringLiteral("geosite-ru"), QStringLiteral("GeoSite RU"), RuleSetKind::GeoSite,
                    QStringLiteral("Used by Bypass RU routing profile.")),
        makeBuiltIn(QStringLiteral("geoip-ru"), QStringLiteral("GeoIP RU"), RuleSetKind::GeoIp,
                    QStringLiteral("Used by Bypass RU routing profile.")),
        makeBuiltIn(QStringLiteral("geosite-cn"), QStringLiteral("GeoSite CN"), RuleSetKind::GeoSite,
                    QStringLiteral("Used by China Direct / Global Remote DNS profile.")),
        makeBuiltIn(QStringLiteral("geoip-cn"), QStringLiteral("GeoIP CN"), RuleSetKind::GeoIp,
                    QStringLiteral("Used by custom DNS expectIPs rules.")),
        makeBuiltIn(QStringLiteral("geosite-geolocation-not-cn"),
                    QStringLiteral("GeoSite geolocation-!cn"), RuleSetKind::GeoSite,
                    QStringLiteral("Used by China Direct / Global Remote DNS profile.")),
        makeBuiltIn(QStringLiteral("geosite-category-ads-all"),
                    QStringLiteral("GeoSite category-ads-all"), RuleSetKind::GeoSite,
                    QStringLiteral("Common ads blocking list reference.")),
    };
}

RuleSetItem* RuleSetCatalog::findByTag(QVector<RuleSetItem>& items, const QString& tag)
{
    for (RuleSetItem& item : items) {
        if (item.tag == tag) {
            return &item;
        }
    }
    return nullptr;
}

const RuleSetItem* RuleSetCatalog::findByTag(const QVector<RuleSetItem>& items,
                                             const QString& tag)
{
    for (const RuleSetItem& item : items) {
        if (item.tag == tag) {
            return &item;
        }
    }
    return nullptr;
}

} // namespace zarya
