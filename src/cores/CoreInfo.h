#pragma once

#include "cores/CoreInstallStatus.h"
#include "domain/CoreType.h"

#include <QDateTime>
#include <QString>

namespace zarya {

struct CoreInfo {
    CoreType type = CoreType::Xray;
    QString name;

    QString executablePath;
    QString installDir;

    bool exists = false;
    bool managed = false;
    bool running = false;

    QString installedVersion;
    QString latestVersion;

    CoreInstallStatus status = CoreInstallStatus::Unknown;
    QString lastError;

    QDateTime lastCheckedAt;
    QDateTime lastUpdatedAt;

    QString selectedAssetName;
    QString checksumStatus;
};

} // namespace zarya
