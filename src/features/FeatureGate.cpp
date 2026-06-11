#include "features/FeatureGate.h"

#include "features/FeatureId.h"
#include "features/FeaturePolicy.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/AppSettings.h"

namespace zarya {

bool FeatureGate::showExperimentalFeatures()
{
    return AppSettings::instance().showExperimentalFeatures();
}

QString FeatureGate::releaseChannelKey()
{
    return AppSettings::instance().releaseChannelKey();
}

bool FeatureGate::isVisible(FeatureId id)
{
    if (!FeaturePolicy::isExperimentalFeature(id)) {
        return true;
    }
    if (id == FeatureId::AppSelfUpdate) {
        const ReleaseChannel channel =
            FeaturePolicy::releaseChannelFromString(releaseChannelKey());
        return showExperimentalFeatures() || !FeaturePolicy::isStableLikeChannel(channel);
    }
    return showExperimentalFeatures();
}

bool FeatureGate::isEnabled(FeatureId id)
{
    if (!isVisible(id)) {
        return false;
    }

    const AppSettings& settings = AppSettings::instance();
    switch (id) {
    case FeatureId::SingBoxTunExperimental:
        return settings.enableExperimentalTun();
    case FeatureId::KillSwitchLinuxNft:
    case FeatureId::KillSwitchWindowsWfp:
        return settings.enableExperimentalKillSwitch();
    case FeatureId::ZaryaHelper:
    case FeatureId::HelperServiceManagement:
        return settings.enableExperimentalTun()
               && settings.tunPrivilegeMode() == TunPrivilegeMode::HelperExperimental;
    case FeatureId::AppSelfUpdate:
        return true;
    case FeatureId::XraySystemProxyRuntime:
    case FeatureId::CoreUpdateManager:
    case FeatureId::BackupImport:
    case FeatureId::DiagnosticsBundle:
        return true;
    }
    return false;
}

bool FeatureGate::experimentalRuntimeConfigured()
{
    const AppSettings& settings = AppSettings::instance();
    return settings.enableExperimentalTun()
           && settings.configuredRuntimeMode() == RuntimeMode::TunSingBoxExperimental;
}

bool FeatureGate::experimentalRuntimeEffective()
{
    const AppSettings& settings = AppSettings::instance();
    return settings.effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental;
}

QJsonObject FeatureGate::diagnosticsJson()
{
    const AppSettings& settings = AppSettings::instance();
    QJsonObject object;
    object.insert(QStringLiteral("releaseChannel"), releaseChannelKey());
    object.insert(QStringLiteral("experimentalFeaturesVisible"), showExperimentalFeatures());

    QJsonObject enabled;
    enabled.insert(QStringLiteral("tun"), isEnabled(FeatureId::SingBoxTunExperimental));
    enabled.insert(QStringLiteral("helper"), isEnabled(FeatureId::ZaryaHelper));
    enabled.insert(QStringLiteral("killSwitch"),
                   isEnabled(FeatureId::KillSwitchLinuxNft)
                       || isEnabled(FeatureId::KillSwitchWindowsWfp));
    object.insert(QStringLiteral("experimentalFeaturesEnabled"), enabled);

    QJsonObject stableScope;
    stableScope.insert(QStringLiteral("recommendedRuntime"), QStringLiteral("XraySystemProxy"));
    object.insert(QStringLiteral("stableScope"), stableScope);

    object.insert(QStringLiteral("configuredRuntimeMode"),
                  runtimeModeToString(settings.configuredRuntimeMode()));
    object.insert(QStringLiteral("effectiveRuntimeMode"),
                  runtimeModeToString(settings.effectiveRuntimeMode()));
    return object;
}

} // namespace zarya
