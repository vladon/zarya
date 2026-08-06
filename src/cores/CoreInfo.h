#pragma once

#include "cores/CoreInstallStatus.h"
#include "domain/CoreType.h"
#include "runtime/core/CoreRuntimeTypes.h"

#include <QDateTime>
#include <QString>

namespace zarya {

struct CoreInfo {
    CoreType type = CoreType::Xray;
    QString name;

    QString executablePath;
    QString installDir;
    QString libraryPath;

    bool exists = false;
    bool managed = false;
    bool running = false;

    CoreDistributionKind distributionKind = CoreDistributionKind::ManagedExecutable;
    CoreRuntimeCapabilities capabilities = CoreRuntimeCapability::Validation
                                           | CoreRuntimeCapability::Update
                                           | CoreRuntimeCapability::Rollback;
    int abiVersion = 0;
    QString loadStatus;

    QString installedVersion;
    QString latestVersion;

    CoreInstallStatus status = CoreInstallStatus::Unknown;
    QString lastError;
    QString lastReleaseCheckError;
    QString lastUpdateError;

    QDateTime lastCheckedAt;
    QDateTime lastUpdatedAt;
    QDateTime lastReleaseCheckAt;

    QString selectedAssetName;
    QString checksumStatus;
};

} // namespace zarya
