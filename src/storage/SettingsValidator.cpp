#include "storage/SettingsValidator.h"

#include "killswitch/KillSwitchMode.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/AppSettings.h"
#include "storage/DefaultSettings.h"

namespace zarya {

namespace {

void clampPort(AppSettings& settings, int (AppSettings::*getter)() const,
                 void (AppSettings::*setter)(int), const QString& label,
                 SettingsValidationResult* result)
{
    const int value = (settings.*getter)();
    if (value < 1 || value > 65535) {
        (settings.*setter)(DefaultSettings::mixedPort());
        result->autoFixed.append(QStringLiteral("%1 port was %2; reset to default.").arg(label).arg(value));
    }
}

} // namespace

SettingsValidationResult SettingsValidator::validateAndFixOnStartup()
{
    SettingsValidationResult result;
    AppSettings& settings = AppSettings::instance();

    clampPort(settings, &AppSettings::mixedPort, &AppSettings::setMixedPort,
              QStringLiteral("Mixed"), &result);

    const int maxTests = settings.maxConcurrentTests();
    if (maxTests < 1 || maxTests > 10) {
        settings.setMaxConcurrentTests(DefaultSettings::maxConcurrentTests());
        result.autoFixed.append(QStringLiteral("Test concurrency was out of range; reset."));
    }

    if (settings.enableExperimentalKillSwitch()
        && settings.tunPrivilegeMode() != TunPrivilegeMode::HelperExperimental) {
        settings.setEnableExperimentalKillSwitch(false);
        result.autoFixed.append(
            QStringLiteral("Kill switch was enabled without helper mode; disabled for beta safety."));
    }

    const QString autoStartId = settings.lastStartedProfileId();
    if (settings.autoStartLastProfile() && autoStartId.isEmpty()) {
        settings.setAutoStartLastProfile(false);
        result.autoFixed.append(QStringLiteral("Auto-start last profile disabled (no profile id)."));
    }

    if (settings.effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental
        && !settings.enableExperimentalTun()) {
        settings.setRuntimeMode(RuntimeMode::SystemProxyXray);
        result.autoFixed.append(QStringLiteral("TUN runtime mode reset to system proxy (TUN disabled)."));
    }

    return result;
}

} // namespace zarya
