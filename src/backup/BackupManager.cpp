#include "backup/BackupManager.h"

#include "backup/BackupExporter.h"
#include "backup/BackupImporter.h"
#include "helperclient/HelperProcessManager.h"
#include "killswitch/KillSwitchState.h"
#include "storage/AppPaths.h"

#include <QFile>

namespace zarya {

BackupManager::BackupManager(QObject* parent)
    : QObject(parent)
{
}

void BackupManager::log(const QString& line)
{
    emit logLine(line);
}

bool BackupManager::isKillSwitchActive(HelperProcessManager* helper)
{
    if (QFile::exists(AppPaths::killSwitchMarkerPath())) {
        return true;
    }
    if (!helper) {
        return false;
    }
    const KillSwitchState state = helper->killSwitchState();
    return state.status == KillSwitchStatus::Enabled
           || state.status == KillSwitchStatus::NeedsRecovery
           || state.recoveryMarkerPresent;
}

QString BackupManager::runtimeBlockReason(bool coreRunning, bool killSwitchActive)
{
    if (killSwitchActive) {
        return QStringLiteral(
            "Kill switch is active. Disable kill switch before importing configuration.");
    }
    if (coreRunning) {
        return QStringLiteral(
            "A proxy core is currently running. Stop it before importing configuration.");
    }
    return {};
}

bool BackupManager::exportBackup(const BackupExportOptions& options, QString* error)
{
    BackupExporter exporter([this](const QString& line) { log(line); });
    return exporter.exportToArchive(options, error);
}

bool BackupManager::loadPreview(const QString& archivePath, BackupManifest* manifest,
                                QString* stagingDir, QString* error)
{
    BackupImporter importer([this](const QString& line) { log(line); });
    return importer.extractAndLoadManifest(archivePath, stagingDir, manifest, error);
}

bool BackupManager::importBackup(const BackupImportOptions& options, const BackupManifest& manifest,
                                 QString* error, QString* preImportBackupPath)
{
    BackupImporter importer([this](const QString& line) { log(line); });
    const BackupImportResult result = importer.importFromStaging(options, manifest, error);
    if (preImportBackupPath) {
        *preImportBackupPath = result.preImportBackupPath;
    }
    if (!result.ok && error && error->isEmpty()) {
        *error = result.message;
    }
    return result.ok;
}

} // namespace zarya
