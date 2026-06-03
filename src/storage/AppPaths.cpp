#include "storage/AppPaths.h"

#include "storage/AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace zarya {

bool AppPaths::s_portableMode = false;

void AppPaths::ensureDir(const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
}

QString AppPaths::applicationDir()
{
    return QCoreApplication::applicationDirPath();
}

QString AppPaths::portableFlagPath()
{
    return QDir(applicationDir()).filePath(QStringLiteral("portable.flag"));
}

void AppPaths::initialize(bool portableRequested)
{
    s_portableMode = portableRequested || QFile::exists(portableFlagPath());
    if (s_portableMode) {
        ensureDir(dataDir());
        ensureDir(runtimeDir());
        ensureDir(coresDir());
    }
}

bool AppPaths::isPortableMode()
{
    return s_portableMode;
}

QString AppPaths::dataDir()
{
    if (s_portableMode) {
        return QDir(applicationDir()).filePath(QStringLiteral("data"));
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    ensureDir(base);
    return base;
}

QString AppPaths::configDir()
{
    return dataDir();
}

QString AppPaths::configFilePath()
{
    if (s_portableMode) {
        return QDir(dataDir()).filePath(QStringLiteral("settings.ini"));
    }
    return {};
}

QString AppPaths::appDataDir()
{
    return dataDir();
}

QString AppPaths::coresDir()
{
    return QDir(applicationDir()).filePath(QStringLiteral("cores"));
}

QString AppPaths::profilesFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("profiles.json"));
}

QString AppPaths::subscriptionsFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("subscriptions.json"));
}

QString AppPaths::routingFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("routing.json"));
}

QString AppPaths::dnsFilePath()
{
    return QDir(dataDir()).filePath(QStringLiteral("dns.json"));
}

QString AppPaths::runtimeDir()
{
    const QString path = s_portableMode ? QDir(applicationDir()).filePath(QStringLiteral("runtime"))
                                        : QDir(dataDir()).filePath(QStringLiteral("runtime"));
    ensureDir(path);
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
    ensureDir(path);
    return path;
}

QString AppPaths::testConfigPath(const QString& profileId)
{
    const QString safeId = profileId.isEmpty() ? QStringLiteral("unknown") : profileId;
    return QDir(testRuntimeDir()).filePath(QStringLiteral("config-%1.json").arg(safeId));
}

QString AppPaths::xrayCoreDir()
{
    return QDir(coresDir()).filePath(QStringLiteral("xray"));
}

QString AppPaths::xrayResourceDir()
{
    const QString xrayPath = AppSettings::instance().resolvedXrayPath().trimmed();
    if (!xrayPath.isEmpty()) {
        return QFileInfo(xrayPath).absolutePath();
    }
    return xrayCoreDir();
}

QString AppPaths::geoDataDir()
{
    if (s_portableMode) {
        return xrayResourceDir();
    }
    const QString path = QDir(dataDir()).filePath(QStringLiteral("geodata"));
    ensureDir(path);
    return path;
}

} // namespace zarya
