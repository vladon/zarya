#include "storage/DefaultSettings.h"

#include "storage/AppSettings.h"

namespace zarya {

int DefaultSettings::socksPort() { return 10808; }
int DefaultSettings::httpPort() { return 10809; }
int DefaultSettings::tcpTestTimeoutMs() { return 5000; }
int DefaultSettings::realDelayTimeoutMs() { return 10000; }
int DefaultSettings::maxConcurrentTests() { return 3; }
int DefaultSettings::autoStartDelaySeconds() { return 3; }
int DefaultSettings::githubApiTimeoutSeconds() { return 20; }
int DefaultSettings::coreBackupRetentionCount() { return 2; }

bool DefaultSettings::autoEnableSystemProxyOnStart()
{
    return AppSettings::defaultAutoEnableSystemProxyOnStart();
}

bool DefaultSettings::restoreProxyOnExit() { return true; }
bool DefaultSettings::minimizeToTrayOnClose() { return AppSettings::defaultMinimizeToTrayOnClose(); }
bool DefaultSettings::enableExperimentalTun() { return false; }
bool DefaultSettings::enableExperimentalKillSwitch() { return false; }
bool DefaultSettings::checkCoreUpdatesOnStartup() { return false; }
bool DefaultSettings::allowCoreUpdateWithoutChecksum() { return false; }

} // namespace zarya
