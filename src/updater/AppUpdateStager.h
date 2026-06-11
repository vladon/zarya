#pragma once

#include <QString>

namespace zarya {

struct AppUpdateStageResult {
    bool ok = false;
    QString stagingDir;
    QString error;
};

class AppUpdateStager {
public:
    static AppUpdateStageResult stageArchive(const QString& archivePath,
                                             const QString& targetVersion,
                                             const QString& expectedManifestVersion);
    static bool isStagedUpdateReady(const QString& stagingDir, const QString& targetVersion,
                                    QString* errorMessage = nullptr);

private:
    static QString normalizeStagingRoot(const QString& extractedDir);
    static bool verifyReleaseManifest(const QString& stagingDir, const QString& expectedVersion,
                                      QString* errorMessage);
    static bool verifyRequiredFiles(const QString& stagingDir, QString* errorMessage);
    static bool hasPathTraversal(const QString& archivePath, QString* errorMessage);
};

} // namespace zarya
