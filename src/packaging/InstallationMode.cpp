#include "packaging/InstallationMode.h"

#include "diagnostics/DiagnosticsRedactor.h"
#include "packaging/WindowsInstallInfo.h"
#include "storage/AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace zarya {

namespace {

QString installedMarkerPath()
{
    return QDir(AppPaths::applicationDir()).filePath(QStringLiteral(".zarya-installed"));
}

bool windowsRegistryInstalled()
{
#if defined(Q_OS_WIN)
    if (!WindowsInstallInfo::isAvailable()) {
        return false;
    }
    const QString installDir = WindowsInstallInfo::installDirectory();
    if (installDir.isEmpty()) {
        return false;
    }
    const QString appDir = QDir::fromNativeSeparators(AppPaths::applicationDir());
    const QString normalizedInstall =
        QDir::fromNativeSeparators(QDir(installDir).absolutePath());
    return appDir.startsWith(normalizedInstall, Qt::CaseInsensitive)
           || WindowsInstallInfo::installerType().contains(QStringLiteral("MSI"),
                                                           Qt::CaseInsensitive);
#else
    return false;
#endif
}

} // namespace

InstallationMode InstallationInfo::detect()
{
    if (QFile::exists(AppPaths::portableFlagPath()) || AppPaths::isPortableMode()) {
        return InstallationMode::Portable;
    }
    if (QFile::exists(installedMarkerPath())) {
        return InstallationMode::Installed;
    }
#if defined(Q_OS_WIN)
    if (windowsRegistryInstalled()) {
        return InstallationMode::Installed;
    }
#endif

    const QString appDir = AppPaths::applicationDir();
#if defined(Q_OS_MACOS)
    if (appDir.contains(QStringLiteral(".app/Contents/MacOS"), Qt::CaseInsensitive)) {
        return InstallationMode::Installed;
    }
#endif
#if defined(Q_OS_WIN)
    if (appDir.contains(QStringLiteral("Program Files"), Qt::CaseInsensitive)) {
        return InstallationMode::Installed;
    }
#endif
#if defined(Q_OS_LINUX)
    if (appDir.startsWith(QStringLiteral("/usr/"))
        || appDir.startsWith(QStringLiteral("/opt/"))) {
        return InstallationMode::Installed;
    }
#endif
    return InstallationMode::Unknown;
}

QString InstallationInfo::modeString(InstallationMode mode)
{
    switch (mode) {
    case InstallationMode::Portable:
        return QStringLiteral("Portable");
    case InstallationMode::Installed:
        return QStringLiteral("Installed");
    case InstallationMode::Unknown:
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

QString InstallationInfo::currentModeString()
{
    return modeString(detect());
}

bool InstallationInfo::portableFlagPresent()
{
    return QFile::exists(AppPaths::portableFlagPath());
}

bool InstallationInfo::installedMarkerPresent()
{
    return QFile::exists(installedMarkerPath());
}

bool InstallationInfo::appDirWritable()
{
    const QFileInfo info(AppPaths::applicationDir());
    return info.isWritable();
}

QJsonObject InstallationInfo::diagnosticsJson()
{
    QJsonObject object;
    object.insert(QStringLiteral("installationMode"), currentModeString());
    object.insert(QStringLiteral("portableFlagPresent"), portableFlagPresent());
    object.insert(QStringLiteral("installedMarkerPresent"), installedMarkerPresent());
    object.insert(QStringLiteral("appDirWritable"), appDirWritable());
    object.insert(QStringLiteral("portableModeActive"), AppPaths::isPortableMode());

#if defined(Q_OS_WIN)
    if (WindowsInstallInfo::isAvailable()) {
        QJsonObject installer;
        installer.insert(QStringLiteral("installer"), WindowsInstallInfo::installerType());
        const QString installDir = WindowsInstallInfo::installDirectory();
        if (!installDir.isEmpty()) {
            installer.insert(QStringLiteral("installDir"),
                             DiagnosticsRedactor::redactText(installDir,
                                                           DiagnosticsRedactionMode::Strict));
        }
        installer.insert(QStringLiteral("registryVersion"), WindowsInstallInfo::registryVersion());
        installer.insert(QStringLiteral("helperServiceInstalled"),
                         WindowsInstallInfo::helperServiceInstalled());
        installer.insert(QStringLiteral("helperServiceState"),
                         WindowsInstallInfo::helperServiceState());
        object.insert(QStringLiteral("windowsInstaller"), installer);
    }
#endif
    return object;
}

} // namespace zarya
