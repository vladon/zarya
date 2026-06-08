#pragma once

#include "backup/BackupExportOptions.h"
#include "backup/BackupImportOptions.h"
#include "backup/BackupManifest.h"

#include <QObject>
#include <QString>

namespace zarya {

class HelperProcessManager;

class BackupManager : public QObject {
    Q_OBJECT

public:
    explicit BackupManager(QObject* parent = nullptr);

    static bool isKillSwitchActive(HelperProcessManager* helper = nullptr);
    static QString runtimeBlockReason(bool coreRunning, bool killSwitchActive);

    bool exportBackup(const BackupExportOptions& options, QString* error);
    bool loadPreview(const QString& archivePath, BackupManifest* manifest, QString* stagingDir,
                     QString* error);
    bool importBackup(const BackupImportOptions& options, const BackupManifest& manifest,
                      QString* error, QString* preImportBackupPath = nullptr);

signals:
    void logLine(const QString& line);

private:
    void log(const QString& line);
};

} // namespace zarya
