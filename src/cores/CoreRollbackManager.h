#pragma once

#include "domain/CoreType.h"

#include <QString>
#include <QStringList>

namespace zarya {

class CoreRollbackManager {
public:
    static QString backupRootDir();
    static QStringList listBackups(CoreType type);
    static bool createBackup(CoreType type, const QString& installDir, const QString& version,
                             QString* errorMessage);
    static bool restoreLatestBackup(CoreType type, const QString& installDir,
                                    QString* restoredVersion, QString* errorMessage);
    static void pruneBackups(CoreType type, int retentionCount);
};

} // namespace zarya
