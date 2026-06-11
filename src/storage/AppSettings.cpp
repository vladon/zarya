#include "storage/AppSettings.h"

#include "app/BuildInfo.h"
#include "app/StartupOptions.h"
#include "platform/Platform.h"
#include "storage/AppPaths.h"

#include <QSettings>

namespace zarya {

AppSettings& AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

QSettings& AppSettings::settings()
{
    if (AppPaths::isPortableMode()) {
        static QSettings portableSettings(AppPaths::configFilePath(), QSettings::IniFormat);
        return portableSettings;
    }
    static QSettings nativeSettings;
    return nativeSettings;
}

bool AppSettings::defaultAutoEnableSystemProxyOnStart()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

QString AppSettings::xrayExecutablePath() const
{
    return settings().value(QStringLiteral("cores/xrayPath")).toString();
}

void AppSettings::setXrayExecutablePath(const QString& path)
{
    settings().setValue(QStringLiteral("cores/xrayPath"), path.trimmed());
}

int AppSettings::socksPort() const
{
    const int port = settings().value(QStringLiteral("proxy/socksPort"), 10808).toInt();
    return port >= 1 && port <= 65535 ? port : 10808;
}

void AppSettings::setSocksPort(int port)
{
    settings().setValue(QStringLiteral("proxy/socksPort"), port);
}

int AppSettings::httpPort() const
{
    const int port = settings().value(QStringLiteral("proxy/httpPort"), 10809).toInt();
    return port >= 1 && port <= 65535 ? port : 10809;
}

void AppSettings::setHttpPort(int port)
{
    settings().setValue(QStringLiteral("proxy/httpPort"), port);
}

QString AppSettings::resolvedXrayPath() const
{
    const QString configured = xrayExecutablePath().trimmed();
    if (!configured.isEmpty()) {
        return configured;
    }
    return Platform::defaultXrayExecutablePath();
}

QString AppSettings::singBoxExecutablePath() const
{
    return settings().value(QStringLiteral("cores/singBoxPath")).toString();
}

void AppSettings::setSingBoxExecutablePath(const QString& path)
{
    settings().setValue(QStringLiteral("cores/singBoxPath"), path.trimmed());
}

QString AppSettings::resolvedSingBoxPath() const
{
    const QString configured = singBoxExecutablePath().trimmed();
    if (!configured.isEmpty()) {
        return configured;
    }
    return Platform::defaultSingBoxExecutablePath();
}

bool AppSettings::enableExperimentalTun() const
{
    return settings().value(QStringLiteral("runtime/enableExperimentalTun"), false).toBool();
}

void AppSettings::setEnableExperimentalTun(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/enableExperimentalTun"), enabled);
}

RuntimeMode AppSettings::runtimeMode() const
{
    return runtimeModeFromString(settings().value(QStringLiteral("runtime/mode")).toString());
}

void AppSettings::setRuntimeMode(RuntimeMode mode)
{
    settings().setValue(QStringLiteral("runtime/mode"), runtimeModeToString(mode));
}

RuntimeMode AppSettings::effectiveRuntimeMode() const
{
    if (!enableExperimentalTun()) {
        return RuntimeMode::SystemProxyXray;
    }
    return runtimeMode();
}

bool AppSettings::tunWarningAccepted() const
{
    return settings().value(QStringLiteral("runtime/tunWarningAccepted"), false).toBool();
}

void AppSettings::setTunWarningAccepted(bool accepted)
{
    settings().setValue(QStringLiteral("runtime/tunWarningAccepted"), accepted);
}

bool AppSettings::experimentalTunWarningAccepted() const
{
    return settings().value(QStringLiteral("runtime/experimentalTunWarningAccepted"), false)
        .toBool();
}

void AppSettings::setExperimentalTunWarningAccepted(bool accepted)
{
    settings().setValue(QStringLiteral("runtime/experimentalTunWarningAccepted"), accepted);
}

void AppSettings::markTunSessionStarted()
{
    settings().setValue(QStringLiteral("runtime/lastShutdownClean"), false);
    settings().setValue(QStringLiteral("runtime/lastRuntimeMode"),
                       runtimeModeToString(RuntimeMode::TunSingBoxExperimental));
    settings().setValue(QStringLiteral("runtime/tunWasRunning"), true);
}

void AppSettings::markCleanShutdown()
{
    settings().setValue(QStringLiteral("runtime/lastShutdownClean"), true);
    settings().setValue(QStringLiteral("runtime/tunWasRunning"), false);
    settings().setValue(QStringLiteral("runtime/lastRuntimeMode"),
                       runtimeModeToString(RuntimeMode::SystemProxyXray));
}

bool AppSettings::shouldWarnUncleanTunShutdown() const
{
    return tunWasRunning() && !lastShutdownClean();
}

bool AppSettings::tunWasRunning() const
{
    return settings().value(QStringLiteral("runtime/tunWasRunning"), false).toBool();
}

bool AppSettings::lastShutdownClean() const
{
    return settings().value(QStringLiteral("runtime/lastShutdownClean"), true).toBool();
}

LogLevel AppSettings::logLevel() const
{
    const QString value = settings().value(QStringLiteral("logging/level"), QStringLiteral("info")).toString();
    if (value.compare(QStringLiteral("debug"), Qt::CaseInsensitive) == 0) {
        return LogLevel::Debug;
    }
    if (value.compare(QStringLiteral("warn"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("warning"), Qt::CaseInsensitive) == 0) {
        return LogLevel::Warn;
    }
    if (value.compare(QStringLiteral("error"), Qt::CaseInsensitive) == 0) {
        return LogLevel::Error;
    }
    return LogLevel::Info;
}

void AppSettings::setLogLevel(LogLevel level)
{
    settings().setValue(QStringLiteral("logging/level"),
                        StartupOptionsParser::logLevelToString(level).toLower());
}

RuntimeMode AppSettings::lastRuntimeMode() const
{
    return runtimeModeFromString(settings().value(QStringLiteral("runtime/lastRuntimeMode")).toString());
}

bool AppSettings::autoEnableSystemProxyOnStart() const
{
    return settings()
        .value(QStringLiteral("proxy/autoEnableSystemProxyOnStart"),
               defaultAutoEnableSystemProxyOnStart())
        .toBool();
}

void AppSettings::setAutoEnableSystemProxyOnStart(bool enabled)
{
    settings().setValue(QStringLiteral("proxy/autoEnableSystemProxyOnStart"), enabled);
}

bool AppSettings::restoreProxyOnExit() const
{
    return settings().value(QStringLiteral("proxy/restoreProxyOnExit"), true).toBool();
}

void AppSettings::setRestoreProxyOnExit(bool enabled)
{
    settings().setValue(QStringLiteral("proxy/restoreProxyOnExit"), enabled);
}

bool AppSettings::confirmBeforeChangingSystemProxy() const
{
    return settings().value(QStringLiteral("proxy/confirmBeforeChangingSystemProxy"), false)
        .toBool();
}

void AppSettings::setConfirmBeforeChangingSystemProxy(bool enabled)
{
    settings().setValue(QStringLiteral("proxy/confirmBeforeChangingSystemProxy"), enabled);
}

QString AppSettings::testUrl() const
{
    return settings()
        .value(QStringLiteral("testing/testUrl"), QStringLiteral("https://www.google.com/generate_204"))
        .toString();
}

void AppSettings::setTestUrl(const QString& url)
{
    settings().setValue(QStringLiteral("testing/testUrl"), url.trimmed());
}

int AppSettings::tcpTestTimeoutMs() const
{
    const int timeout = settings().value(QStringLiteral("testing/tcpTimeoutMs"), 5000).toInt();
    return qBound(1000, timeout, 60000);
}

void AppSettings::setTcpTestTimeoutMs(int timeoutMs)
{
    settings().setValue(QStringLiteral("testing/tcpTimeoutMs"), qBound(1000, timeoutMs, 60000));
}

int AppSettings::realDelayTimeoutMs() const
{
    const int timeout = settings().value(QStringLiteral("testing/realDelayTimeoutMs"), 10000).toInt();
    return qBound(1000, timeout, 60000);
}

void AppSettings::setRealDelayTimeoutMs(int timeoutMs)
{
    settings().setValue(QStringLiteral("testing/realDelayTimeoutMs"),
                       qBound(1000, timeoutMs, 60000));
}

int AppSettings::maxConcurrentTests() const
{
    const int count = settings().value(QStringLiteral("testing/maxConcurrentTests"), 3).toInt();
    return qBound(1, count, 10);
}

void AppSettings::setMaxConcurrentTests(int count)
{
    settings().setValue(QStringLiteral("testing/maxConcurrentTests"), qBound(1, count, 10));
}

bool AppSettings::skipTcpBeforeRealDelay() const
{
    return settings().value(QStringLiteral("testing/skipTcpBeforeRealDelay"), false).toBool();
}

void AppSettings::setSkipTcpBeforeRealDelay(bool skip)
{
    settings().setValue(QStringLiteral("testing/skipTcpBeforeRealDelay"), skip);
}

bool AppSettings::defaultMinimizeToTrayOnClose()
{
    return true;
}

bool AppSettings::minimizeToTrayOnClose() const
{
    return settings()
        .value(QStringLiteral("desktop/minimizeToTrayOnClose"), defaultMinimizeToTrayOnClose())
        .toBool();
}

void AppSettings::setMinimizeToTrayOnClose(bool enabled)
{
    settings().setValue(QStringLiteral("desktop/minimizeToTrayOnClose"), enabled);
}

bool AppSettings::minimizeToTrayOnMinimize() const
{
    return settings().value(QStringLiteral("desktop/minimizeToTrayOnMinimize"), false).toBool();
}

void AppSettings::setMinimizeToTrayOnMinimize(bool enabled)
{
    settings().setValue(QStringLiteral("desktop/minimizeToTrayOnMinimize"), enabled);
}

bool AppSettings::showTrayNotifications() const
{
    return settings().value(QStringLiteral("desktop/showTrayNotifications"), true).toBool();
}

void AppSettings::setShowTrayNotifications(bool enabled)
{
    settings().setValue(QStringLiteral("desktop/showTrayNotifications"), enabled);
}

bool AppSettings::confirmExitWhileRunning() const
{
    return settings().value(QStringLiteral("desktop/confirmExitWhileRunning"), true).toBool();
}

void AppSettings::setConfirmExitWhileRunning(bool enabled)
{
    settings().setValue(QStringLiteral("desktop/confirmExitWhileRunning"), enabled);
}

QString AppSettings::selectedRoutingProfileId() const
{
    return settings().value(QStringLiteral("routing/selectedProfileId")).toString();
}

void AppSettings::setSelectedRoutingProfileId(const QString& profileId)
{
    settings().setValue(QStringLiteral("routing/selectedProfileId"), profileId.trimmed());
}

QString AppSettings::selectedDnsProfileId() const
{
    return settings().value(QStringLiteral("dns/selectedProfileId")).toString();
}

void AppSettings::setSelectedDnsProfileId(const QString& profileId)
{
    settings().setValue(QStringLiteral("dns/selectedProfileId"), profileId.trimmed());
}

bool AppSettings::macApplyProxyToAllServices() const
{
    return settings().value(QStringLiteral("proxy/macApplyToAllServices"), false).toBool();
}

void AppSettings::setMacApplyProxyToAllServices(bool enabled)
{
    settings().setValue(QStringLiteral("proxy/macApplyToAllServices"), enabled);
}

QString AppSettings::macPreferredNetworkService() const
{
    return settings().value(QStringLiteral("proxy/macPreferredNetworkService")).toString();
}

void AppSettings::setMacPreferredNetworkService(const QString& service)
{
    settings().setValue(QStringLiteral("proxy/macPreferredNetworkService"), service.trimmed());
}

bool AppSettings::startMinimizedToTray() const
{
    return settings().value(QStringLiteral("startup/startMinimizedToTray"), false).toBool();
}

void AppSettings::setStartMinimizedToTray(bool enabled)
{
    settings().setValue(QStringLiteral("startup/startMinimizedToTray"), enabled);
}

bool AppSettings::autoStartLastProfile() const
{
    return settings().value(QStringLiteral("startup/autoStartLastProfile"), false).toBool();
}

void AppSettings::setAutoStartLastProfile(bool enabled)
{
    settings().setValue(QStringLiteral("startup/autoStartLastProfile"), enabled);
}

bool AppSettings::autoEnableSystemProxyAfterAutoStart() const
{
    return settings().value(QStringLiteral("startup/autoEnableSystemProxyAfterAutoStart"), true)
        .toBool();
}

void AppSettings::setAutoEnableSystemProxyAfterAutoStart(bool enabled)
{
    settings().setValue(QStringLiteral("startup/autoEnableSystemProxyAfterAutoStart"), enabled);
}

int AppSettings::autoStartDelaySeconds() const
{
    const int seconds = settings().value(QStringLiteral("startup/autoStartDelaySeconds"), 3).toInt();
    return qBound(0, seconds, 120);
}

void AppSettings::setAutoStartDelaySeconds(int seconds)
{
    settings().setValue(QStringLiteral("startup/autoStartDelaySeconds"), qBound(0, seconds, 120));
}

QString AppSettings::lastStartedProfileId() const
{
    return settings().value(QStringLiteral("startup/lastStartedProfileId")).toString();
}

void AppSettings::setLastStartedProfileId(const QString& profileId)
{
    settings().setValue(QStringLiteral("startup/lastStartedProfileId"), profileId.trimmed());
}

bool AppSettings::tunUseActiveRoutingProfile() const
{
    return settings().value(QStringLiteral("runtime/tunUseActiveRoutingProfile"), true).toBool();
}

void AppSettings::setTunUseActiveRoutingProfile(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/tunUseActiveRoutingProfile"), enabled);
}

bool AppSettings::tunUseActiveDnsProfile() const
{
    return settings().value(QStringLiteral("runtime/tunUseActiveDnsProfile"), true).toBool();
}

void AppSettings::setTunUseActiveDnsProfile(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/tunUseActiveDnsProfile"), enabled);
}

bool AppSettings::tunEnableDnsHijack() const
{
    return settings().value(QStringLiteral("runtime/tunEnableDnsHijack"), true).toBool();
}

void AppSettings::setTunEnableDnsHijack(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/tunEnableDnsHijack"), enabled);
}

TunDnsHijackMode AppSettings::tunDnsHijackMode() const
{
    return tunDnsHijackModeFromString(
        settings().value(QStringLiteral("runtime/tunDnsHijackMode")).toString());
}

void AppSettings::setTunDnsHijackMode(TunDnsHijackMode mode)
{
    settings().setValue(QStringLiteral("runtime/tunDnsHijackMode"), tunDnsHijackModeToString(mode));
}

TunPrivilegeMode AppSettings::tunPrivilegeMode() const
{
    return tunPrivilegeModeFromString(
        settings().value(QStringLiteral("runtime/tunPrivilegeMode")).toString());
}

void AppSettings::setTunPrivilegeMode(TunPrivilegeMode mode)
{
    settings().setValue(QStringLiteral("runtime/tunPrivilegeMode"), tunPrivilegeModeToString(mode));
}

bool AppSettings::enableExperimentalKillSwitch() const
{
    return settings().value(QStringLiteral("runtime/enableExperimentalKillSwitch"), false).toBool();
}

void AppSettings::setEnableExperimentalKillSwitch(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/enableExperimentalKillSwitch"), enabled);
}

KillSwitchMode AppSettings::killSwitchMode() const
{
    return killSwitchModeFromString(
        settings().value(QStringLiteral("runtime/killSwitchMode")).toString());
}

void AppSettings::setKillSwitchMode(KillSwitchMode mode)
{
    settings().setValue(QStringLiteral("runtime/killSwitchMode"), killSwitchModeToString(mode));
}

bool AppSettings::killSwitchAllowLan() const
{
    return settings().value(QStringLiteral("runtime/killSwitchAllowLan"), true).toBool();
}

void AppSettings::setKillSwitchAllowLan(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/killSwitchAllowLan"), enabled);
}

bool AppSettings::killSwitchAllowLoopback() const
{
    return settings().value(QStringLiteral("runtime/killSwitchAllowLoopback"), true).toBool();
}

void AppSettings::setKillSwitchAllowLoopback(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/killSwitchAllowLoopback"), enabled);
}

bool AppSettings::killSwitchBlockWhenTunStopped() const
{
    return settings().value(QStringLiteral("runtime/killSwitchBlockWhenTunStopped"), true).toBool();
}

void AppSettings::setKillSwitchBlockWhenTunStopped(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/killSwitchBlockWhenTunStopped"), enabled);
}

bool AppSettings::killSwitchAutoDisableOnCleanStop() const
{
    return settings().value(QStringLiteral("runtime/killSwitchAutoDisableOnCleanStop"), true)
        .toBool();
}

void AppSettings::setKillSwitchAutoDisableOnCleanStop(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/killSwitchAutoDisableOnCleanStop"), enabled);
}

bool AppSettings::tunRequireLocalRuleSets() const
{
    return settings().value(QStringLiteral("runtime/tunRequireLocalRuleSets"), false).toBool();
}

void AppSettings::setTunRequireLocalRuleSets(bool enabled)
{
    settings().setValue(QStringLiteral("runtime/tunRequireLocalRuleSets"), enabled);
}

bool AppSettings::startAtLogin() const
{
    return settings().value(QStringLiteral("startup/startAtLogin"), false).toBool();
}

void AppSettings::setStartAtLogin(bool enabled)
{
    settings().setValue(QStringLiteral("startup/startAtLogin"), enabled);
}

bool AppSettings::allowCoreUpdateWithoutChecksum() const
{
    return settings().value(QStringLiteral("cores/allowUpdateWithoutChecksum"), false).toBool();
}

void AppSettings::setAllowCoreUpdateWithoutChecksum(bool enabled)
{
    settings().setValue(QStringLiteral("cores/allowUpdateWithoutChecksum"), enabled);
}

bool AppSettings::allowManageExternalCorePaths() const
{
    return settings().value(QStringLiteral("cores/allowManageExternalCorePaths"), false).toBool();
}

void AppSettings::setAllowManageExternalCorePaths(bool enabled)
{
    settings().setValue(QStringLiteral("cores/allowManageExternalCorePaths"), enabled);
}

int AppSettings::coreBackupRetentionCount() const
{
    const int count = settings().value(QStringLiteral("cores/backupRetentionCount"), 2).toInt();
    return count >= 1 ? count : 2;
}

void AppSettings::setCoreBackupRetentionCount(int count)
{
    settings().setValue(QStringLiteral("cores/backupRetentionCount"), qMax(1, count));
}

int AppSettings::githubApiTimeoutSeconds() const
{
    const int seconds = settings().value(QStringLiteral("cores/githubApiTimeoutSeconds"), 20).toInt();
    return seconds >= 5 ? seconds : 20;
}

void AppSettings::setGithubApiTimeoutSeconds(int seconds)
{
    settings().setValue(QStringLiteral("cores/githubApiTimeoutSeconds"), qMax(5, seconds));
}

bool AppSettings::checkCoreUpdatesOnStartup() const
{
    return settings().value(QStringLiteral("cores/checkUpdatesOnStartup"), false).toBool();
}

void AppSettings::setCheckCoreUpdatesOnStartup(bool enabled)
{
    settings().setValue(QStringLiteral("cores/checkUpdatesOnStartup"), enabled);
}

QString AppSettings::appUpdateChannelKey() const
{
    const QString configured =
        settings().value(QStringLiteral("app/updateChannel")).toString().trimmed();
    if (!configured.isEmpty()) {
        return configured;
    }
    return BuildInfo::buildChannel();
}

void AppSettings::setAppUpdateChannelKey(const QString& channel)
{
    settings().setValue(QStringLiteral("app/updateChannel"), channel.trimmed().toLower());
}

bool AppSettings::checkAppUpdatesOnStartup() const
{
    return settings().value(QStringLiteral("app/checkUpdatesOnStartup"), false).toBool();
}

void AppSettings::setCheckAppUpdatesOnStartup(bool enabled)
{
    settings().setValue(QStringLiteral("app/checkUpdatesOnStartup"), enabled);
}

QString AppSettings::appUpdateManifestUrl() const
{
    return settings().value(QStringLiteral("app/updateManifestUrl")).toString().trimmed();
}

void AppSettings::setAppUpdateManifestUrl(const QString& url)
{
    settings().setValue(QStringLiteral("app/updateManifestUrl"), url.trimmed());
}

bool AppSettings::allowUnsignedAppUpdates() const
{
    return settings().value(QStringLiteral("app/allowUnsignedAppUpdates"), false).toBool();
}

void AppSettings::setAllowUnsignedAppUpdates(bool enabled)
{
    settings().setValue(QStringLiteral("app/allowUnsignedAppUpdates"), enabled);
}

bool AppSettings::firstRunCompleted() const
{
    return settings().value(QStringLiteral("onboarding/firstRunCompleted"), false).toBool();
}

void AppSettings::setFirstRunCompleted(bool completed)
{
    settings().setValue(QStringLiteral("onboarding/firstRunCompleted"), completed);
}

bool AppSettings::firstRunCoreInstalled() const
{
    return settings().value(QStringLiteral("onboarding/firstRunCoreInstalled"), false).toBool();
}

void AppSettings::setFirstRunCoreInstalled(bool installed)
{
    settings().setValue(QStringLiteral("onboarding/firstRunCoreInstalled"), installed);
}

bool AppSettings::firstRunProfilesImported() const
{
    return settings()
        .value(QStringLiteral("onboarding/firstRunProfilesImported"), false)
        .toBool();
}

void AppSettings::setFirstRunProfilesImported(bool imported)
{
    settings().setValue(QStringLiteral("onboarding/firstRunProfilesImported"), imported);
}

bool AppSettings::dismissBetaBanner() const
{
    return settings().value(QStringLiteral("onboarding/dismissBetaBanner"), false).toBool();
}

void AppSettings::setDismissBetaBanner(bool dismissed)
{
    settings().setValue(QStringLiteral("onboarding/dismissBetaBanner"), dismissed);
}

QString AppSettings::languageCode() const
{
    return settings().value(QStringLiteral("desktop/languageCode"), QStringLiteral("system")).toString();
}

void AppSettings::setLanguageCode(const QString& code)
{
    settings().setValue(QStringLiteral("desktop/languageCode"), code);
}

} // namespace zarya
