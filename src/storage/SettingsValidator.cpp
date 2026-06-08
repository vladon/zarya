#include "storage/SettingsValidator.h"

#include "killswitch/KillSwitchMode.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/AppSettings.h"
#include "storage/DefaultSettings.h"

#include <QFile>

namespace zarya {

namespace {

void clampPort(AppSettings& settings, int (AppSettings::*getter)() const,
                 void (AppSettings::*setter)(int), const QString& label,
                 SettingsValidationResult* result)
{
    const int value = (settings.*getter)();
    if (value < 1 || value > 65535) {
        (settings.*setter)(label.contains(QStringLiteral("HTTP")) ? DefaultSettings::httpPort()
                                                                    : DefaultSettings::socksPort());
        result->autoFixed.append(QStringLiteral("%1 port was %2; reset to default.").arg(label).arg(value));
    }
}

} // namespace

SettingsValidationResult SettingsValidator::validateAndFixOnStartup()
{
    SettingsValidationResult result;
    AppSettings& settings = AppSettings::instance();

    clampPort(settings, &AppSettings::socksPort, &AppSettings::setSocksPort,
              QStringLiteral("SOCKS"), &result);
    clampPort(settings, &AppSettings::httpPort, &AppSettings::setHttpPort, QStringLiteral("HTTP"),
              &result);

    if (settings.socksPort() == settings.httpPort()) {
        settings.setHttpPort(DefaultSettings::httpPort());
        result.autoFixed.append(QStringLiteral("HTTP and SOCKS ports were equal; HTTP reset."));
    }

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

    const QString xrayPath = settings.resolvedXrayPath();
    if (!xrayPath.isEmpty() && !QFile::exists(xrayPath)) {
        result.warnings.append(QStringLiteral("Xray executable not found at configured path."));
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
