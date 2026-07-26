#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

class DefaultSettings {
public:
    static int mixedPort();
    static int tcpTestTimeoutMs();
    static int realDelayTimeoutMs();
    static int maxConcurrentTests();
    static int autoStartDelaySeconds();
    static int githubApiTimeoutSeconds();
    static int coreBackupRetentionCount();

    /// Default real-delay probe URL (Cloudflare trace endpoint).
    static QString testUrl();
    /// Built-in presets offered in Settings (first entry matches testUrl()).
    static QStringList testUrlPresets();

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
