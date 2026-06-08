#pragma once

#include "app/StartupOptions.h"
#include "killswitch/KillSwitchMode.h"
#include "runtime/RuntimeBackendType.h"

#include <QString>

class QSettings;

namespace zarya {

class AppSettings {
public:
    static AppSettings& instance();

    static QSettings& settings();

    QString xrayExecutablePath() const;
    void setXrayExecutablePath(const QString& path);

    int socksPort() const;
    void setSocksPort(int port);

    int httpPort() const;
    void setHttpPort(int port);

    QString resolvedXrayPath() const;

    QString singBoxExecutablePath() const;
    void setSingBoxExecutablePath(const QString& path);
    QString resolvedSingBoxPath() const;

    bool enableExperimentalTun() const;
    void setEnableExperimentalTun(bool enabled);

    RuntimeMode runtimeMode() const;
    void setRuntimeMode(RuntimeMode mode);
    RuntimeMode effectiveRuntimeMode() const;

    bool tunWarningAccepted() const;
    void setTunWarningAccepted(bool accepted);

    void markTunSessionStarted();
    void markCleanShutdown();
    bool shouldWarnUncleanTunShutdown() const;
    bool tunWasRunning() const;
    bool lastShutdownClean() const;
    RuntimeMode lastRuntimeMode() const;

    LogLevel logLevel() const;
    void setLogLevel(LogLevel level);

    bool autoEnableSystemProxyOnStart() const;
    void setAutoEnableSystemProxyOnStart(bool enabled);

    bool restoreProxyOnExit() const;
    void setRestoreProxyOnExit(bool enabled);

    bool confirmBeforeChangingSystemProxy() const;
    void setConfirmBeforeChangingSystemProxy(bool enabled);

    QString testUrl() const;
    void setTestUrl(const QString& url);

    int tcpTestTimeoutMs() const;
    void setTcpTestTimeoutMs(int timeoutMs);

    int realDelayTimeoutMs() const;
    void setRealDelayTimeoutMs(int timeoutMs);

    int maxConcurrentTests() const;
    void setMaxConcurrentTests(int count);

    bool skipTcpBeforeRealDelay() const;
    void setSkipTcpBeforeRealDelay(bool skip);

    bool minimizeToTrayOnClose() const;
    void setMinimizeToTrayOnClose(bool enabled);

    bool minimizeToTrayOnMinimize() const;
    void setMinimizeToTrayOnMinimize(bool enabled);

    bool showTrayNotifications() const;
    void setShowTrayNotifications(bool enabled);

    bool confirmExitWhileRunning() const;
    void setConfirmExitWhileRunning(bool enabled);

    QString selectedRoutingProfileId() const;
    void setSelectedRoutingProfileId(const QString& profileId);

    QString selectedDnsProfileId() const;
    void setSelectedDnsProfileId(const QString& profileId);

    bool macApplyProxyToAllServices() const;
    void setMacApplyProxyToAllServices(bool enabled);

    QString macPreferredNetworkService() const;
    void setMacPreferredNetworkService(const QString& service);

    bool startMinimizedToTray() const;
    void setStartMinimizedToTray(bool enabled);

    bool autoStartLastProfile() const;
    void setAutoStartLastProfile(bool enabled);

    bool autoEnableSystemProxyAfterAutoStart() const;
    void setAutoEnableSystemProxyAfterAutoStart(bool enabled);

    int autoStartDelaySeconds() const;
    void setAutoStartDelaySeconds(int seconds);

    QString lastStartedProfileId() const;
    void setLastStartedProfileId(const QString& profileId);

    bool tunUseActiveRoutingProfile() const;
    void setTunUseActiveRoutingProfile(bool enabled);

    bool tunUseActiveDnsProfile() const;
    void setTunUseActiveDnsProfile(bool enabled);

    bool tunEnableDnsHijack() const;
    void setTunEnableDnsHijack(bool enabled);

    TunDnsHijackMode tunDnsHijackMode() const;
    void setTunDnsHijackMode(TunDnsHijackMode mode);

    TunPrivilegeMode tunPrivilegeMode() const;
    void setTunPrivilegeMode(TunPrivilegeMode mode);

    bool enableExperimentalKillSwitch() const;
    void setEnableExperimentalKillSwitch(bool enabled);

    KillSwitchMode killSwitchMode() const;
    void setKillSwitchMode(KillSwitchMode mode);

    bool killSwitchAllowLan() const;
    void setKillSwitchAllowLan(bool enabled);

    bool killSwitchAllowLoopback() const;
    void setKillSwitchAllowLoopback(bool enabled);

    bool killSwitchBlockWhenTunStopped() const;
    void setKillSwitchBlockWhenTunStopped(bool enabled);

    bool killSwitchAutoDisableOnCleanStop() const;
    void setKillSwitchAutoDisableOnCleanStop(bool enabled);

    bool tunRequireLocalRuleSets() const;
    void setTunRequireLocalRuleSets(bool enabled);

    bool startAtLogin() const;
    void setStartAtLogin(bool enabled);

    bool allowCoreUpdateWithoutChecksum() const;
    void setAllowCoreUpdateWithoutChecksum(bool enabled);

    bool allowManageExternalCorePaths() const;
    void setAllowManageExternalCorePaths(bool enabled);

    int coreBackupRetentionCount() const;
    void setCoreBackupRetentionCount(int count);

    int githubApiTimeoutSeconds() const;
    void setGithubApiTimeoutSeconds(int seconds);

    bool checkCoreUpdatesOnStartup() const;
    void setCheckCoreUpdatesOnStartup(bool enabled);

    bool firstRunCompleted() const;
    void setFirstRunCompleted(bool completed);

    bool dismissBetaBanner() const;
    void setDismissBetaBanner(bool dismissed);

    static bool defaultAutoEnableSystemProxyOnStart();
    static bool defaultMinimizeToTrayOnClose();

private:
    AppSettings() = default;
};

} // namespace zarya
