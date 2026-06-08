#pragma once

#include "domain/CoreType.h"

#include <QString>

namespace zarya {

class CorePaths {
public:
    static QString managedInstallDir(CoreType type);
    static QString managedExecutablePath(CoreType type);
    static bool isManagedExecutablePath(const QString& executablePath, CoreType type);
    static QString coreUpdatesDir();
    static QString coreUpdatesDownloadsDir();
    static QString coreUpdatesExtractDir();
    static QString metadataFilePath(const QString& installDir);
    static QString versionFilePath(const QString& installDir);
    static void ensureUpdateDirs();
};

} // namespace zarya
