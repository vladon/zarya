#include "backup/BackupCategory.h"

namespace zarya {

QString backupCategoryKey(BackupCategory category)
{
    switch (category) {
    case BackupCategory::Profiles:
        return QStringLiteral("profiles");
    case BackupCategory::Subscriptions:
        return QStringLiteral("subscriptions");
    case BackupCategory::RoutingProfiles:
        return QStringLiteral("routing");
    case BackupCategory::DnsProfiles:
        return QStringLiteral("dns");
    case BackupCategory::AppSettings:
        return QStringLiteral("settings");
    case BackupCategory::GeoDataSettings:
        return QStringLiteral("geoDataSettings");
    case BackupCategory::SingBoxRuleSetMetadata:
        return QStringLiteral("ruleSets");
    case BackupCategory::SingBoxRuleSetFiles:
        return QStringLiteral("ruleSetFiles");
    case BackupCategory::XrayGeoDataFiles:
        return QStringLiteral("geoData");
    case BackupCategory::CoreMetadata:
        return QStringLiteral("coreMetadata");
    case BackupCategory::LogsRedacted:
        return QStringLiteral("logs");
    }
    return {};
}

QString backupCategoryDisplayName(BackupCategory category)
{
    switch (category) {
    case BackupCategory::Profiles:
        return QStringLiteral("Profiles");
    case BackupCategory::Subscriptions:
        return QStringLiteral("Subscriptions");
    case BackupCategory::RoutingProfiles:
        return QStringLiteral("Routing profiles");
    case BackupCategory::DnsProfiles:
        return QStringLiteral("DNS profiles");
    case BackupCategory::AppSettings:
        return QStringLiteral("App settings");
    case BackupCategory::GeoDataSettings:
        return QStringLiteral("Geo data settings");
    case BackupCategory::SingBoxRuleSetMetadata:
        return QStringLiteral("sing-box rule-set metadata");
    case BackupCategory::SingBoxRuleSetFiles:
        return QStringLiteral("sing-box .srs files");
    case BackupCategory::XrayGeoDataFiles:
        return QStringLiteral("Xray geoip.dat/geosite.dat");
    case BackupCategory::CoreMetadata:
        return QStringLiteral("Core metadata");
    case BackupCategory::LogsRedacted:
        return QStringLiteral("Redacted logs");
    }
    return {};
}

BackupCategory backupCategoryFromKey(const QString& key)
{
    const QVector<BackupCategory> all = {
        BackupCategory::Profiles,       BackupCategory::Subscriptions,
        BackupCategory::RoutingProfiles, BackupCategory::DnsProfiles,
        BackupCategory::AppSettings,    BackupCategory::GeoDataSettings,
        BackupCategory::SingBoxRuleSetMetadata, BackupCategory::SingBoxRuleSetFiles,
        BackupCategory::XrayGeoDataFiles, BackupCategory::CoreMetadata,
        BackupCategory::LogsRedacted,
    };
    for (BackupCategory category : all) {
        if (backupCategoryKey(category).compare(key, Qt::CaseInsensitive) == 0) {
            return category;
        }
    }
    return BackupCategory::Profiles;
}

QVector<BackupCategory> defaultExportCategories()
{
    return {BackupCategory::Profiles,
            BackupCategory::Subscriptions,
            BackupCategory::RoutingProfiles,
            BackupCategory::DnsProfiles,
            BackupCategory::AppSettings,
            BackupCategory::GeoDataSettings,
            BackupCategory::SingBoxRuleSetMetadata,
            BackupCategory::CoreMetadata};
}

bool isLargeBackupCategory(BackupCategory category)
{
    return category == BackupCategory::SingBoxRuleSetFiles
           || category == BackupCategory::XrayGeoDataFiles;
}

} // namespace zarya
