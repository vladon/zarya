#include "storage/AppPaths.h"

#include <QDir>
#include <QStandardPaths>

namespace zarya {

QString AppPaths::appDataDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.absolutePath();
}

QString AppPaths::profilesFilePath()
{
    return QDir(appDataDir()).filePath(QStringLiteral("profiles.json"));
}

QString AppPaths::subscriptionsFilePath()
{
    return QDir(appDataDir()).filePath(QStringLiteral("subscriptions.json"));
}

QString AppPaths::runtimeDir()
{
    const QString path = QDir(appDataDir()).filePath(QStringLiteral("runtime"));
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return path;
}

QString AppPaths::xrayConfigPath()
{
    return QDir(runtimeDir()).filePath(QStringLiteral("config-xray.json"));
}

QString AppPaths::singBoxConfigPath()
{
    return QDir(runtimeDir()).filePath(QStringLiteral("config-singbox.json"));
}

QString AppPaths::testRuntimeDir()
{
    const QString path = QDir(runtimeDir()).filePath(QStringLiteral("test"));
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return path;
}

QString AppPaths::testConfigPath(const QString& profileId)
{
    const QString safeId = profileId.isEmpty() ? QStringLiteral("unknown") : profileId;
    return QDir(testRuntimeDir()).filePath(QStringLiteral("config-%1.json").arg(safeId));
}

} // namespace zarya
