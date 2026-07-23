#include "cores/CoreBinaryManager.h"

#include "core/CoreManager.h"
#include "cores/CoreArchiveExtractor.h"
#include "cores/CoreAssetSelector.h"
#include "cores/CoreChecksum.h"
#include "cores/CoreDownloader.h"
#include "cores/CoreInstaller.h"
#include "cores/CorePaths.h"
#include "cores/CorePlatform.h"
#include "cores/CoreRollbackManager.h"
#include "cores/CoreVerifier.h"
#include "cores/CoreVersionDetector.h"
#include "cores/GitHubReleaseProvider.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>

namespace zarya {

namespace {

CoreInfo makeBaseInfo(CoreType type)
{
    CoreInfo info;
    info.type = type;
    info.name = type == CoreType::Xray ? QStringLiteral("Xray") : QStringLiteral("sing-box");
    info.installDir = CorePaths::managedInstallDir(type);
    info.executablePath = CorePaths::managedExecutablePath(type);
    return info;
}

QString userAgent()
{
    return QStringLiteral("Zarya/%1").arg(PackagingInfo::versionString());
}

} // namespace

CoreBinaryManager::CoreBinaryManager(QObject* parent)
    : QObject(parent)
{
    m_infos = {makeBaseInfo(CoreType::Xray), makeBaseInfo(CoreType::SingBox)};
    m_downloader = new CoreDownloader(this);
}

void CoreBinaryManager::setProcessCoreManager(CoreManager* coreManager)
{
    m_processCoreManager = coreManager;
}

void CoreBinaryManager::setSingBoxRunningCallback(const std::function<bool()>& callback)
{
    m_singBoxRunningCallback = callback;
}

QVector<CoreInfo> CoreBinaryManager::coreInfos() const
{
    return m_infos;
}

CoreInfo CoreBinaryManager::infoFor(CoreType type) const
{
    for (const CoreInfo& info : m_infos) {
        if (info.type == type) {
            return info;
        }
    }
    return makeBaseInfo(type);
}

void CoreBinaryManager::emitChanged()
{
    emit coresChanged(m_infos);
}

void CoreBinaryManager::refreshLocalState()
{
    for (CoreInfo& info : m_infos) {
        info.executablePath = CorePaths::managedExecutablePath(info.type);
        info.installDir = QFileInfo(info.executablePath).absolutePath();
        info.exists = QFile::exists(info.executablePath);
        const QString configuredPath = info.type == CoreType::Xray
                                           ? AppSettings::instance().xrayExecutablePath().trimmed()
                                           : AppSettings::instance().singBoxExecutablePath().trimmed();
        const bool pathIsManaged = CorePaths::isManagedExecutablePath(info.executablePath, info.type);
        if (!configuredPath.isEmpty() && !pathIsManaged
            && !AppSettings::instance().allowManageExternalCorePaths()) {
            info.managed = false;
            info.status = CoreInstallStatus::External;
        } else {
            info.managed = true;
        }

        if (isCoreRunning(info.type)) {
            info.running = true;
            info.status = CoreInstallStatus::Running;
        } else if (!info.exists) {
            info.status = CoreInstallStatus::Missing;
            info.installedVersion.clear();
        } else {
            info.running = false;
            const DetectedVersion detected =
                CoreVersionDetector::detect(info.executablePath, info.type);
            if (detected.ok) {
                info.installedVersion = detected.version;
                if (info.status != CoreInstallStatus::External) {
                    info.status = CoreInstallStatus::Installed;
                }
            } else {
                info.status = CoreInstallStatus::Unknown;
                info.lastError = detected.error;
            }
        }

        const CoreRelease latest = m_latestReleases.value(info.type);
        if (!latest.version.isEmpty()) {
            info.latestVersion = CoreVersionDetector::normalizeVersion(latest.version);
            if (info.exists && !info.running && info.managed && info.status != CoreInstallStatus::External
                && !info.installedVersion.isEmpty()
                && info.installedVersion != info.latestVersion) {
                info.status = CoreInstallStatus::UpdateAvailable;
            }
        }
    }
    emitChanged();
}

bool CoreBinaryManager::isCoreRunning(CoreType type) const
{
    if (!m_processCoreManager || !m_processCoreManager->isRunning()) {
        if (type == CoreType::SingBox && m_singBoxRunningCallback) {
            return m_singBoxRunningCallback();
        }
        return false;
    }
    const QString runningName = m_processCoreManager->runningCoreName();
    if (type == CoreType::Xray) {
        return runningName.compare(QStringLiteral("Xray"), Qt::CaseInsensitive) == 0;
    }
    if (runningName.compare(QStringLiteral("sing-box"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    return type == CoreType::SingBox && m_singBoxRunningCallback && m_singBoxRunningCallback();
}

bool CoreBinaryManager::canManage(CoreType type, QString* reason) const
{
    const CoreInfo info = infoFor(type);
    if (isCoreRunning(type)) {
        if (reason) {
            *reason = QStringLiteral("Cannot update %1 while it is running. Stop the core first.")
                          .arg(info.name);
        }
        return false;
    }
    if (!info.managed) {
        if (reason) {
            *reason = QStringLiteral(
                "Core path is external and not managed by Zarya. Reset to managed path or enable "
                "external core management in settings.");
        }
        return false;
    }
    return true;
}

void CoreBinaryManager::finishVersionChecks()
{
    refreshLocalState();

    QStringList errors;
    for (const CoreInfo& info : m_infos) {
        if (!info.lastReleaseCheckError.isEmpty()) {
            errors.append(QStringLiteral("%1: %2").arg(info.name, info.lastReleaseCheckError));
        }
    }

    if (errors.isEmpty()) {
        emit operationFinished(true, QString());
    } else {
        emit operationFinished(false, errors.join(QStringLiteral("\n")));
    }
}

void CoreBinaryManager::checkLatestVersions()
{
    m_pendingChecks = 2;
    const int timeoutMs = AppSettings::instance().githubApiTimeoutSeconds() * 1000;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (CoreInfo& info : m_infos) {
        info.lastReleaseCheckAt = now;
        info.lastReleaseCheckError.clear();
    }

    auto* xrayProvider = new GitHubReleaseProvider(CoreType::Xray, QStringLiteral("XTLS/Xray-core"), this);
    connect(xrayProvider, &CoreReleaseProvider::latestReleaseReady, this,
            [this, xrayProvider](const CoreRelease& release) {
                m_latestReleases.insert(CoreType::Xray, release);
                for (CoreInfo& info : m_infos) {
                    if (info.type == CoreType::Xray) {
                        info.lastReleaseCheckAt = QDateTime::currentDateTimeUtc();
                        info.lastReleaseCheckError.clear();
                    }
                }
                emit logLine(QStringLiteral("Fetching latest Xray release: %1").arg(release.version));
                --m_pendingChecks;
                if (m_pendingChecks <= 0) {
                    finishVersionChecks();
                }
                xrayProvider->deleteLater();
            });
    connect(xrayProvider, &CoreReleaseProvider::error, this, [this, xrayProvider](const QString& message) {
        for (CoreInfo& info : m_infos) {
            if (info.type == CoreType::Xray) {
                info.lastReleaseCheckError = message;
            }
        }
        emit logLine(QStringLiteral("Xray release check failed: %1").arg(message));
        --m_pendingChecks;
        if (m_pendingChecks <= 0) {
            finishVersionChecks();
        }
        xrayProvider->deleteLater();
    });
    emit logLine(QStringLiteral("Checking Xray version"));
    xrayProvider->fetchLatestRelease(timeoutMs);

    auto* singBoxProvider =
        new GitHubReleaseProvider(CoreType::SingBox, QStringLiteral("SagerNet/sing-box"), this);
    connect(singBoxProvider, &CoreReleaseProvider::latestReleaseReady, this,
            [this, singBoxProvider](const CoreRelease& release) {
                m_latestReleases.insert(CoreType::SingBox, release);
                for (CoreInfo& info : m_infos) {
                    if (info.type == CoreType::SingBox) {
                        info.lastReleaseCheckAt = QDateTime::currentDateTimeUtc();
                        info.lastReleaseCheckError.clear();
                    }
                }
                emit logLine(QStringLiteral("Fetching latest sing-box release: %1").arg(release.version));
                --m_pendingChecks;
                if (m_pendingChecks <= 0) {
                    finishVersionChecks();
                }
                singBoxProvider->deleteLater();
            });
    connect(singBoxProvider, &CoreReleaseProvider::error, this,
            [this, singBoxProvider](const QString& message) {
                for (CoreInfo& info : m_infos) {
                    if (info.type == CoreType::SingBox) {
                        info.lastReleaseCheckError = message;
                    }
                }
                emit logLine(QStringLiteral("sing-box release check failed: %1").arg(message));
                --m_pendingChecks;
                if (m_pendingChecks <= 0) {
                    finishVersionChecks();
                }
                singBoxProvider->deleteLater();
            });
    emit logLine(QStringLiteral("Checking sing-box version"));
    singBoxProvider->fetchLatestRelease(timeoutMs);

    refreshLocalState();
}

void CoreBinaryManager::updateCore(CoreType type, bool allowMissingChecksum)
{
    QString reason;
    if (!canManage(type, &reason)) {
        emit operationFinished(false, reason);
        return;
    }

    const CoreRelease release = m_latestReleases.value(type);
    if (release.version.isEmpty() || release.assets.isEmpty()) {
        emit operationFinished(false, QStringLiteral("Latest release metadata is not available. Check versions first."));
        return;
    }

    handleReleaseReady(type, release, allowMissingChecksum);
}

void CoreBinaryManager::updateAll(bool allowMissingChecksum)
{
    if (m_latestReleases.value(CoreType::Xray).version.isEmpty()
        || m_latestReleases.value(CoreType::SingBox).version.isEmpty()) {
        emit operationFinished(false, QStringLiteral("Check versions before updating all cores."));
        return;
    }
    updateCore(CoreType::Xray, allowMissingChecksum);
    updateCore(CoreType::SingBox, allowMissingChecksum);
}

void CoreBinaryManager::handleReleaseReady(CoreType type, const CoreRelease& release,
                                           bool allowMissingChecksum)
{
    const auto selected = CoreAssetSelector::selectBest(release, currentCorePlatform());
    if (!selected.has_value()) {
        emit operationFinished(false, QStringLiteral("No suitable release asset found for this platform."));
        return;
    }

    for (CoreInfo& info : m_infos) {
        if (info.type == type) {
            info.status = CoreInstallStatus::Updating;
            info.selectedAssetName = selected->name;
        }
    }
    emitChanged();
    emit logLine(QStringLiteral("Selected asset: %1").arg(selected->name));
    continueDownload(type, release, *selected, release.assets, allowMissingChecksum);
}

void CoreBinaryManager::continueDownload(CoreType type, const CoreRelease& release,
                                         const CoreAsset& asset, const QVector<CoreAsset>& allAssets,
                                         bool allowMissingChecksum)
{
    CorePaths::ensureUpdateDirs();
    const QString archivePath =
        QDir(CorePaths::coreUpdatesDownloadsDir())
            .filePath(QStringLiteral("%1-%2").arg(coreTypeToString(type), asset.name));

    m_activeDownloadType = type;
    emit logLine(QStringLiteral("Downloading core archive"));

    // downloadToFile is synchronous and emits finished before returning; do not wait on a
    // QEventLoop after the call (exec() would hang forever after quit already ran).
    disconnect(m_downloader, &CoreDownloader::progress, this, nullptr);
    connect(m_downloader, &CoreDownloader::progress, this,
            [this](qint64 received, qint64 total) {
                emit downloadProgress(m_activeDownloadType, received, total);
            });
    QString downloadError;
    const bool downloadOk =
        m_downloader->downloadToFile(asset.downloadUrl, archivePath, userAgent(),
                                     AppSettings::instance().githubApiTimeoutSeconds() * 1000,
                                     &downloadError);

    if (!downloadOk) {
        emit operationFinished(false, downloadError);
        refreshLocalState();
        return;
    }

    const auto checksumAssetName = CoreChecksum::findChecksumAssetName(allAssets, asset.name);
    if (!checksumAssetName.has_value()) {
        if (!allowMissingChecksum
            && !AppSettings::instance().allowCoreUpdateWithoutChecksum()) {
            emit operationFinished(
                false, QStringLiteral("Checksum file was not found for this asset. Enable "
                                      "allow-without-checksum in settings or retry later."));
            refreshLocalState();
            return;
        }
        emit logLine(QStringLiteral("Checksum unavailable; continuing with warning"));
        continueAfterArchiveDownload(type, release, archivePath, allAssets, allowMissingChecksum,
                                     false);
        return;
    }

    CoreAsset checksumAsset;
    for (const CoreAsset& candidate : allAssets) {
        if (candidate.name == *checksumAssetName) {
            checksumAsset = candidate;
            break;
        }
    }

    const QString checksumPath = QDir(CorePaths::coreUpdatesDownloadsDir()).filePath(checksumAsset.name);
    emit logLine(QStringLiteral("Downloading checksum"));
    CoreDownloader checksumDownloader;
    QString checksumError;
    const bool checksumOk = checksumDownloader.downloadToFile(
        checksumAsset.downloadUrl, checksumPath, userAgent(),
        AppSettings::instance().githubApiTimeoutSeconds() * 1000, &checksumError);

    if (!checksumOk) {
        emit operationFinished(false, checksumError);
        refreshLocalState();
        return;
    }

    QFile checksumFile(checksumPath);
    checksumFile.open(QIODevice::ReadOnly);
    const auto expected = CoreChecksum::parseExpectedSha256(checksumFile.readAll(), asset.name);
    if (!expected.has_value()) {
        emit operationFinished(false, QStringLiteral("Could not parse checksum file."));
        refreshLocalState();
        return;
    }

    QString verifyError;
    if (!CoreChecksum::verifyFileSha256(archivePath, *expected, &verifyError)) {
        emit operationFinished(false, verifyError);
        refreshLocalState();
        return;
    }
    emit logLine(QStringLiteral("Checksum verified"));
    continueAfterArchiveDownload(type, release, archivePath, allAssets, allowMissingChecksum, true);
}

void CoreBinaryManager::continueAfterArchiveDownload(CoreType type, const CoreRelease& release,
                                                     const QString& archivePath,
                                                     const QVector<CoreAsset>& /*allAssets*/,
                                                     bool /*allowMissingChecksum*/,
                                                     bool /*checksumVerified*/)
{
    const QString extractDir =
        QDir(CorePaths::coreUpdatesExtractDir())
            .filePath(QStringLiteral("%1-%2")
                          .arg(coreTypeToString(type),
                               CoreVersionDetector::normalizeVersion(release.version)));

    QDir(extractDir).removeRecursively();
    emit logLine(QStringLiteral("Extracting archive"));
    const ExtractResult extracted = CoreArchiveExtractor::extract(archivePath, extractDir);
    if (!extracted.ok) {
        emit operationFinished(false, extracted.error);
        refreshLocalState();
        return;
    }

    const QString stagedExecutable = CoreVerifier::findExecutableInTree(extractDir, type);
    emit logLine(QStringLiteral("Verifying staged executable"));
    QString verifyError;
    if (!CoreVerifier::verifyStaged(stagedExecutable, type, release.version, &verifyError)) {
        emit operationFinished(false, verifyError);
        refreshLocalState();
        return;
    }

    finishInstall(type, release, archivePath);
}

void CoreBinaryManager::finishInstall(CoreType type, const CoreRelease& release,
                                      const QString& /*archivePath*/)
{
    const QString extractDir =
        QDir(CorePaths::coreUpdatesExtractDir())
            .filePath(QStringLiteral("%1-%2")
                          .arg(coreTypeToString(type),
                               CoreVersionDetector::normalizeVersion(release.version)));
    const QString stagedExecutable = CoreVerifier::findExecutableInTree(extractDir, type);
    const QString installDir = CorePaths::managedInstallDir(type);

    emit logLine(QStringLiteral("Backing up current core"));
    emit logLine(QStringLiteral("Installing new core"));
    QString installError;
    if (!CoreInstaller::installFromStaging(type, stagedExecutable, installDir, release.version,
                                           &installError)) {
        emit operationFinished(false, installError);
        refreshLocalState();
        return;
    }

    CoreRollbackManager::pruneBackups(type, AppSettings::instance().coreBackupRetentionCount());
    emit logLine(QStringLiteral("Final verification OK"));
    emit logLine(QStringLiteral("Core update completed"));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (CoreInfo& info : m_infos) {
        if (info.type == type) {
            info.lastUpdatedAt = now;
            info.lastUpdateError.clear();
        }
    }

    if (type == CoreType::Xray) {
        AppSettings::instance().setXrayExecutablePath({});
    } else {
        AppSettings::instance().setSingBoxExecutablePath({});
    }

    refreshLocalState();
    emit operationFinished(true, QStringLiteral("Core update completed."));
}

void CoreBinaryManager::rollback(CoreType type)
{
    QString reason;
    if (!canManage(type, &reason)) {
        emit operationFinished(false, reason);
        return;
    }

    emit logLine(QStringLiteral("Rollback started"));
    QString restoredVersion;
    QString error;
    if (!CoreRollbackManager::restoreLatestBackup(type, CorePaths::managedInstallDir(type),
                                                  &restoredVersion, &error)) {
        emit operationFinished(false, error);
        return;
    }

    const QString executable = CorePaths::managedExecutablePath(type);
    if (!CoreVerifier::verifyStaged(executable, type, restoredVersion, &error)) {
        emit operationFinished(false, error);
        return;
    }

    emit logLine(QStringLiteral("Rollback completed"));
    refreshLocalState();
    emit operationFinished(true, QStringLiteral("Rollback completed."));
}

void CoreBinaryManager::cancelDownload()
{
    if (m_downloader) {
        m_downloader->cancel();
    }
}

bool CoreBinaryManager::setManagedExecutablePath(CoreType type)
{
    const QString path = CorePaths::managedExecutablePath(type);
    if (type == CoreType::Xray) {
        AppSettings::instance().setXrayExecutablePath(path);
    } else {
        AppSettings::instance().setSingBoxExecutablePath(path);
    }
    refreshLocalState();
    return true;
}

void CoreBinaryManager::resetToManagedPath(CoreType type)
{
    if (type == CoreType::Xray) {
        AppSettings::instance().setXrayExecutablePath({});
    } else {
        AppSettings::instance().setSingBoxExecutablePath({});
    }
    refreshLocalState();
}

} // namespace zarya
