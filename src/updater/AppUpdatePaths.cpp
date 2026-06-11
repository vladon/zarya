#include "updater/AppUpdatePaths.h"

#include "storage/AppPaths.h"

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

void AppUpdatePaths::ensureDirectories()
{
    QDir().mkpath(downloadsDir());
}

} // namespace zarya
