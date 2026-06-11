#pragma once

#include <QString>

namespace zarya {

class AppUpdatePaths {
public:
    static QString appUpdatesRootDir();
    static QString downloadsDir();
    static QString stagingRootDir();
    static QString backupsRootDir();
    static QString logsDir();
    static QString pendingUpdatePath();
    static QString updateSuccessPath();
    static QString updateFailedPath();
    static QString stagingDirForVersion(const QString& targetVersion);
    static QString backupDirForVersion(const QString& targetVersion);
    static void ensureDirectories();
};

} // namespace zarya
