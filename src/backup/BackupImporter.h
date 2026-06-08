#pragma once

#include "backup/BackupImportOptions.h"
#include "backup/BackupManifest.h"

#include "domain/DnsProfile.h"
#include "domain/Profile.h"
#include "domain/RoutingProfile.h"
#include "domain/Subscription.h"

#include <functional>
#include <QString>

namespace zarya {

struct BackupImportResult {
    bool ok = false;
    QString message;
    QString preImportBackupPath;
};

class BackupImporter {
public:
    using LogCallback = std::function<void(const QString&)>;

    explicit BackupImporter(LogCallback log = {});

    bool extractAndLoadManifest(const QString& archivePath, QString* stagingDir, BackupManifest* manifest,
                                QString* error);
    BackupImportResult importFromStaging(const BackupImportOptions& options,
                                         const BackupManifest& manifest, QString* error);

private:
    bool createPreImportBackup(QString* backupPath, QString* error);
    bool importProfiles(const QString& filePath, ImportMode mode, QString* error);
    bool importSubscriptions(const QString& filePath, ImportMode mode, QString* error);
    bool importRouting(const QString& filePath, ImportMode mode, QString* error);
    bool importDns(const QString& filePath, ImportMode mode, QString* error);
    bool importSettings(const QString& filePath, bool machineSpecific, QString* error);
    bool importGeoDataSettings(const QString& filePath, QString* error);
    bool importRuleSetMetadata(const QString& filePath, ImportMode mode, QString* error);
    bool importRuleSetFiles(const QString& sourceDir, QString* error);
    bool importGeoFiles(const QString& sourceDir, QString* error);
    bool importCoreMetadata(const QString& filePath, QString* error);

    LogCallback m_log;
};

} // namespace zarya
