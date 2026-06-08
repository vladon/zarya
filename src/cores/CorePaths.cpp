#include "cores/CorePaths.h"

#include "cores/CoreVerifier.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QDir>
#include <QFileInfo>

namespace zarya {

QString CorePaths::managedInstallDir(CoreType type)
{
    return type == CoreType::Xray ? AppPaths::xrayCoreDir() : AppPaths::singBoxCoreDir();
}

QString CorePaths::managedExecutablePath(CoreType type)
{
    const AppSettings& settings = AppSettings::instance();
    const QString configured = type == CoreType::Xray ? settings.xrayExecutablePath().trimmed()
                                                      : settings.singBoxExecutablePath().trimmed();
    if (!configured.isEmpty()) {
        return configured;
    }
    return QDir(managedInstallDir(type)).filePath(CoreVerifier::executableFileName(type));
}

bool CorePaths::isManagedExecutablePath(const QString& executablePath, CoreType type)
{
    if (executablePath.trimmed().isEmpty()) {
        return true;
    }
    const QString managedDir = QFileInfo(managedInstallDir(type)).absoluteFilePath();
    const QString actualPath = QFileInfo(executablePath).absoluteFilePath();
    return actualPath.startsWith(managedDir);
}

QString CorePaths::coreUpdatesDir()
{
    return QDir(AppPaths::runtimeDir()).filePath(QStringLiteral("core-updates"));
}

QString CorePaths::coreUpdatesDownloadsDir()
{
    const QString path = QDir(coreUpdatesDir()).filePath(QStringLiteral("downloads"));
    QDir().mkpath(path);
    return path;
}

QString CorePaths::coreUpdatesExtractDir()
{
    const QString path = QDir(coreUpdatesDir()).filePath(QStringLiteral("extract"));
    QDir().mkpath(path);
    return path;
}

QString CorePaths::metadataFilePath(const QString& installDir)
{
    return QDir(installDir).filePath(QStringLiteral(".zarya-core.json"));
}

QString CorePaths::versionFilePath(const QString& installDir)
{
    return QDir(installDir).filePath(QStringLiteral("VERSION"));
}

void CorePaths::ensureUpdateDirs()
{
    QDir().mkpath(coreUpdatesDownloadsDir());
    QDir().mkpath(coreUpdatesExtractDir());
}

} // namespace zarya
