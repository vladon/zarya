#pragma once

#include <QString>
#include <QVector>

namespace zarya {

enum class BackupCategory {
    Profiles,
    Subscriptions,
    RoutingProfiles,
    DnsProfiles,
    AppSettings,
    GeoDataSettings,
    SingBoxRuleSetMetadata,
    SingBoxRuleSetFiles,
    XrayGeoDataFiles,
    CoreMetadata,
    LogsRedacted,
};

QString backupCategoryKey(BackupCategory category);
QString backupCategoryDisplayName(BackupCategory category);
BackupCategory backupCategoryFromKey(const QString& key);

QVector<BackupCategory> defaultExportCategories();
bool isLargeBackupCategory(BackupCategory category);

} // namespace zarya
