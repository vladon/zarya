#include "updater/AppUpdatePaths.h"

#include "storage/AppPaths.h"

#include <QDateTime>
#include <QDir>

namespace zarya {

QString AppUpdatePaths::appUpdatesRootDir()
{
    return QDir(AppPaths::runtimeDir()).filePath(QStringLiteral("app-updates"));
}

QString AppUpdatePaths::downloadsDir()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("downloads"));
}

QString AppUpdatePaths::stagingRootDir()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("staging"));
}

QString AppUpdatePaths::backupsRootDir()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("backups"));
}

QString AppUpdatePaths::logsDir()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("logs"));
}

QString AppUpdatePaths::pendingUpdatePath()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("pending-update.json"));
}

QString AppUpdatePaths::updateSuccessPath()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("update-success.json"));
}

QString AppUpdatePaths::updateFailedPath()
{
    return QDir(appUpdatesRootDir()).filePath(QStringLiteral("update-failed.json"));
}

QString AppUpdatePaths::stagingDirForVersion(const QString& targetVersion)
{
    const QString safeVersion = targetVersion;
    return QDir(stagingRootDir()).filePath(QStringLiteral("Zarya-%1").arg(safeVersion));
}

QString AppUpdatePaths::backupDirForVersion(const QString& targetVersion)
{
    const QString timestamp =
        QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return QDir(backupsRootDir())
        .filePath(QStringLiteral("pre-%1-%2").arg(targetVersion, timestamp));
}

void AppUpdatePaths::ensureDirectories()
{
    QDir().mkpath(downloadsDir());
    QDir().mkpath(stagingRootDir());
    QDir().mkpath(backupsRootDir());
    QDir().mkpath(logsDir());
}

} // namespace zarya
