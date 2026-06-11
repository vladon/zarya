#include "updater/AppUpdateStager.h"

#include "backup/BackupArchive.h"
#include "cores/CoreArchiveExtractor.h"
#include "updater/AppUpdatePaths.h"
#include "updater/AppVersion.h"
#include "updater/runner/UpdatePlanFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

bool removeRecursivelyIfExists(const QString& path)
{
    if (!QFileInfo(path).exists()) {
        return true;
    }
    if (QFileInfo(path).isDir()) {
        return QDir(path).removeRecursively();
    }
    return QFile::remove(path);
}

} // namespace

bool AppUpdateStager::hasPathTraversal(const QString& archivePath, QString* errorMessage)
{
    return !CoreArchiveExtractor::validateArchiveEntries(archivePath, errorMessage);
}

QString AppUpdateStager::normalizeStagingRoot(const QString& extractedDir)
{
    QDir dir(extractedDir);
    const QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs);
    if (entries.size() == 1) {
        const QString candidate = dir.filePath(entries.first());
        if (QFileInfo(candidate).isDir()) {
            return candidate;
        }
    }
    return extractedDir;
}

bool AppUpdateStager::verifyReleaseManifest(const QString& stagingDir,
                                            const QString& expectedVersion,
                                            QString* errorMessage)
{
    const QString manifestPath = QDir(stagingDir).filePath(QStringLiteral("release-manifest.json"));
    if (!QFile::exists(manifestPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staged update is missing release-manifest.json.");
        }
        return false;
    }

    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to read staged release-manifest.json.");
        }
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staged release-manifest.json is invalid.");
        }
        return false;
    }

    const QString stagedVersion = doc.object().value(QStringLiteral("version")).toString();
    if (AppVersion::compare(stagedVersion, expectedVersion) != 0) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("Staged release-manifest version mismatch. Expected %1, got %2.")
                    .arg(expectedVersion, stagedVersion);
        }
        return false;
    }
    return true;
}

bool AppUpdateStager::verifyRequiredFiles(const QString& stagingDir, QString* errorMessage)
{
    const QStringList required = {
        UpdatePlan::mainExecutableName(),
        UpdatePlan::helperExecutableName(),
        UpdatePlan::updaterExecutableName(),
    };
    for (const QString& name : required) {
        if (!QFile::exists(QDir(stagingDir).filePath(name))) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Staged update is missing required file: %1")
                                    .arg(name);
            }
            return false;
        }
    }

    const QDir translationsDir(QDir(stagingDir).filePath(QStringLiteral("translations")));
    if (!translationsDir.exists() || translationsDir.entryList(QDir::Files).isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staged update is missing translations.");
        }
        return false;
    }

    const QDir docsDir(QDir(stagingDir).filePath(QStringLiteral("docs")));
    if (!docsDir.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staged update is missing docs.");
        }
        return false;
    }
    return true;
}

bool AppUpdateStager::isStagedUpdateReady(const QString& stagingDir, const QString& targetVersion,
                                          QString* errorMessage)
{
    if (!QDir(stagingDir).exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staging directory does not exist.");
        }
        return false;
    }
    if (!verifyReleaseManifest(stagingDir, targetVersion, errorMessage)) {
        return false;
    }
    return verifyRequiredFiles(stagingDir, errorMessage);
}

AppUpdateStageResult AppUpdateStager::stageArchive(const QString& archivePath,
                                                  const QString& targetVersion,
                                                  const QString& expectedManifestVersion)
{
    AppUpdateStageResult result;
    AppUpdatePaths::ensureDirectories();

    QString traversalError;
    if (!hasPathTraversal(archivePath, &traversalError)) {
        result.error = traversalError.isEmpty()
                           ? QStringLiteral("Archive contains unsafe paths.")
                           : traversalError;
        return result;
    }

    const QString finalStagingDir = AppUpdatePaths::stagingDirForVersion(targetVersion);
    const QString tempExtractDir =
        QDir(AppUpdatePaths::stagingRootDir())
            .filePath(QStringLiteral(".extract-%1").arg(targetVersion));

    removeRecursivelyIfExists(tempExtractDir);
    removeRecursivelyIfExists(finalStagingDir);
    QDir().mkpath(tempExtractDir);

    if (archivePath.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
        const BackupArchiveResult extracted = BackupArchive::extractZip(archivePath, tempExtractDir);
        if (!extracted.ok) {
            result.error = extracted.error;
            removeRecursivelyIfExists(tempExtractDir);
            return result;
        }
    } else {
        const ExtractResult extracted = CoreArchiveExtractor::extract(archivePath, tempExtractDir);
        if (!extracted.ok) {
            result.error = extracted.error;
            removeRecursivelyIfExists(tempExtractDir);
            return result;
        }
    }

    const QString normalizedRoot = normalizeStagingRoot(tempExtractDir);
    QString verifyError;
    const QString manifestVersion =
        expectedManifestVersion.isEmpty() ? targetVersion : expectedManifestVersion;
    if (!verifyReleaseManifest(normalizedRoot, manifestVersion, &verifyError)
        || !verifyRequiredFiles(normalizedRoot, &verifyError)) {
        result.error = verifyError;
        removeRecursivelyIfExists(tempExtractDir);
        return result;
    }

    removeRecursivelyIfExists(finalStagingDir);
    if (!QDir().rename(normalizedRoot, finalStagingDir)) {
        result.error = QStringLiteral("Failed to finalize staging directory.");
        removeRecursivelyIfExists(tempExtractDir);
        return result;
    }
    if (normalizedRoot != tempExtractDir) {
        removeRecursivelyIfExists(tempExtractDir);
    }

    result.ok = true;
    result.stagingDir = finalStagingDir;
    return result;
}

} // namespace zarya
