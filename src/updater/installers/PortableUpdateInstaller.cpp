#include "updater/installers/PortableUpdateInstaller.h"

#include "app/BuildInfo.h"
#include "packaging/InstallationMode.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "updater/AppUpdatePaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSysInfo>

namespace zarya {

bool PortableUpdateInstaller::isImplemented()
{
#if defined(Q_OS_MACOS)
    return false;
#else
    return !isAppImageMode();
#endif
}

QString PortableUpdateInstaller::statusMessage()
{
    if (InstallationInfo::currentMode() == InstallationMode::Installed) {
        return QStringLiteral(
            "This Zarya installation is managed by an installer.\n"
            "Automatic installed-mode updates are not implemented yet.\n"
            "Download and verify the installer manually, then run it.");
    }
#if defined(Q_OS_MACOS)
    return QStringLiteral(
        "macOS app bundle self-replacement is not implemented in 0.35.\n"
        "Download and verify the update artifact manually.");
#endif
    if (isAppImageMode()) {
        return QStringLiteral(
            "AppImage self-update is not implemented in 0.35.\n"
            "Download and verify the update artifact manually.");
    }
    return QStringLiteral("Ready to install portable update.\nZarya will close and restart.");
}

bool PortableUpdateInstaller::isAppImageMode()
{
#if defined(Q_OS_LINUX)
    return !qEnvironmentVariableIsEmpty("APPIMAGE");
#else
    return false;
#endif
}

bool PortableUpdateInstaller::canInstallPortableUpdate(const AppUpdateAsset& asset,
                                                       bool artifactVerified, bool stagingReady,
                                                       bool runtimeRunning, bool testsRunning,
                                                       bool killSwitchActive, QString* reason)
{
    auto setReason = [&](const QString& text) {
        if (reason) {
            *reason = text;
        }
        return false;
    };

    if (!AppSettings::instance().enablePortableUpdaterPoC()
        && !AppSettings::instance().allowDevLocalAppUpdateInstall()) {
        const QString channel = BuildInfo::buildChannel();
        if (channel == QStringLiteral("rc")) {
            return setReason(QStringLiteral(
                "Self-update installation is experimental and disabled in this RC build.\n"
                "You can download and verify updates manually."));
        }
        return setReason(QStringLiteral(
            "Self-update installation is experimental and disabled in this build.\n"
            "You can download and verify updates manually."));
    }
    if (InstallationInfo::currentMode() != InstallationMode::Portable) {
        return setReason(QStringLiteral(
            "Automatic updates are not available for installed mode yet."));
    }
    if (!isImplemented()) {
        return setReason(statusMessage());
    }
    if (asset.installationMode != QStringLiteral("portable")) {
        return setReason(QStringLiteral("Selected asset is not a portable update."));
    }
    if (asset.sha256.isEmpty()) {
        return setReason(QStringLiteral("Update artifact checksum is missing."));
    }
    if (!artifactVerified) {
        return setReason(QStringLiteral("Download the update and verify SHA256 first."));
    }
    if (!stagingReady) {
        return setReason(QStringLiteral("Update staging is not ready."));
    }
    if (runtimeRunning) {
        return setReason(QStringLiteral("Stop the running profile before installing an update."));
    }
    if (testsRunning) {
        return setReason(QStringLiteral("Wait for tests to finish before installing an update."));
    }
    if (killSwitchActive) {
        return setReason(QStringLiteral("Disable kill switch before installing an update."));
    }
    return true;
}

namespace {

QString currentPlatformToken()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return QStringLiteral("unknown");
#endif
}

QString currentArchitectureToken()
{
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    if (arch.contains(QStringLiteral("arm64")) || arch.contains(QStringLiteral("aarch64"))) {
        return QStringLiteral("arm64");
    }
    return QStringLiteral("x64");
}

} // namespace

UpdatePlan PortableUpdateInstaller::buildUpdatePlan(const QString& currentVersion,
                                                      const QString& targetVersion,
                                                      const QString& stagingDir)
{
    UpdatePlan plan;
    plan.currentVersion = currentVersion;
    plan.targetVersion = targetVersion;
    plan.installationMode = QStringLiteral("Portable");
    plan.platform = currentPlatformToken();
    plan.architecture = currentArchitectureToken();
    plan.appDir = AppPaths::applicationDir();
    plan.stagingDir = stagingDir;
    plan.backupDir = AppUpdatePaths::backupDirForVersion(targetVersion);
    plan.preservePaths = UpdatePlan::defaultPreservePaths();
    plan.mainExecutable = UpdatePlan::mainExecutableName();
    plan.helperExecutable = UpdatePlan::helperExecutableName();
    plan.updaterExecutable = UpdatePlan::updaterExecutableName();
    plan.postUpdateCheck.command = plan.mainExecutable;
    plan.postUpdateCheck.args = {QStringLiteral("--version")};
    plan.postUpdateCheck.expectedVersion = targetVersion;
    plan.restartAfterUpdate = true;
    plan.restartArgs = {QStringLiteral("--post-update")};
    return plan;
}

QString PortableUpdateInstaller::resolvedUpdaterPath()
{
    const QString updaterName = UpdatePlan::updaterExecutableName();
    const QString localPath = QDir(AppPaths::applicationDir()).filePath(updaterName);
    if (QFileInfo::exists(localPath)) {
        return localPath;
    }
    return localPath;
}

bool PortableUpdateInstaller::launchUpdaterAndQuit(const UpdatePlan& plan,
                                                   QString* errorMessage)
{
    AppUpdatePaths::ensureDirectories();
    const QString planPath = AppUpdatePaths::pendingUpdatePath();
    if (!UpdatePlan::writeFile(planPath, plan, errorMessage)) {
        return false;
    }

    const QString updaterPath = resolvedUpdaterPath();
    if (!QFileInfo::exists(updaterPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("zarya-updater executable was not found.");
        }
        return false;
    }

    const bool started =
        QProcess::startDetached(updaterPath, {QStringLiteral("--plan"), planPath}, plan.appDir);
    if (!started && errorMessage) {
        *errorMessage = QStringLiteral("Failed to launch zarya-updater.");
    }
    return started;
}

} // namespace zarya
