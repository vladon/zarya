#pragma once

namespace zarya {

class DefaultSettings {
public:
    static int socksPort();
    static int httpPort();
    static int tcpTestTimeoutMs();
    static int realDelayTimeoutMs();
    static int maxConcurrentTests();
    static int autoStartDelaySeconds();
    static int githubApiTimeoutSeconds();
    static int coreBackupRetentionCount();

    static bool autoEnableSystemProxyOnStart();
    static bool restoreProxyOnExit();
    static bool minimizeToTrayOnClose();
    static bool enableExperimentalTun();
    static bool enableExperimentalKillSwitch();
    static bool checkCoreUpdatesOnStartup();
    static bool checkAppUpdatesOnStartup();
    static bool allowCoreUpdateWithoutChecksum();
    static bool allowUnsignedAppUpdates();
    static int appUpdateBackupRetentionCount();
    static bool showExperimentalFeatures();
    static bool enablePortableUpdaterPoC();
};

} // namespace zarya
