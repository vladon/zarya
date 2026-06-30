#pragma once

#include "cores/CoreInfo.h"
#include "cores/CoreRelease.h"
#include "domain/CoreType.h"

#include <QObject>
#include <functional>

namespace zarya {

class CoreManager;
class CoreDownloader;

class CoreBinaryManager : public QObject {
    Q_OBJECT

public:
    explicit CoreBinaryManager(QObject* parent = nullptr);

    void setProcessCoreManager(CoreManager* coreManager);
    void setSingBoxRunningCallback(const std::function<bool()>& callback);

    QVector<CoreInfo> coreInfos() const;
    CoreInfo infoFor(CoreType type) const;

    void refreshLocalState();
    void checkLatestVersions();
    void updateCore(CoreType type, bool allowMissingChecksum = false);
    void updateAll(bool allowMissingChecksum = false);
    void rollback(CoreType type);
    void cancelDownload();

    bool setManagedExecutablePath(CoreType type);
    void resetToManagedPath(CoreType type);

signals:
    void logLine(const QString& line);
    void coresChanged(const QVector<CoreInfo>& infos);
    void downloadProgress(CoreType type, qint64 received, qint64 total);
    void operationFinished(bool ok, const QString& message);

private:
    void emitChanged();
    bool isCoreRunning(CoreType type) const;
    bool canManage(CoreType type, QString* reason) const;
    void handleReleaseReady(CoreType type, const CoreRelease& release,
                            bool allowMissingChecksum);
    void continueDownload(CoreType type, const CoreRelease& release, const CoreAsset& asset,
                          const QVector<CoreAsset>& allAssets, bool allowMissingChecksum);
    void continueAfterArchiveDownload(CoreType type, const CoreRelease& release,
                                      const QString& archivePath,
                                      const QVector<CoreAsset>& allAssets,
                                      bool allowMissingChecksum, bool checksumVerified);
    void finishInstall(CoreType type, const CoreRelease& release, const QString& archivePath);
    void finishVersionChecks();

    CoreManager* m_processCoreManager = nullptr;
    std::function<bool()> m_singBoxRunningCallback;
    QVector<CoreInfo> m_infos;
    QHash<CoreType, CoreRelease> m_latestReleases;
    CoreDownloader* m_downloader = nullptr;
    CoreType m_activeDownloadType = CoreType::Xray;
    int m_pendingChecks = 0;
};

} // namespace zarya
