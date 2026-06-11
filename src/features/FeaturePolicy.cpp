#include "features/FeaturePolicy.h"

#include "app/BuildInfo.h"

namespace zarya {

ReleaseChannel FeaturePolicy::releaseChannelFromString(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower == QStringLiteral("dev")) {
        return ReleaseChannel::Dev;
    }
    if (lower == QStringLiteral("rc")) {
        return ReleaseChannel::Rc;
    }
    if (lower == QStringLiteral("stable")) {
        return ReleaseChannel::Stable;
    }
    return ReleaseChannel::Beta;
}

QString FeaturePolicy::releaseChannelToString(ReleaseChannel channel)
{
    switch (channel) {
    case ReleaseChannel::Dev:
        return QStringLiteral("dev");
    case ReleaseChannel::Rc:
        return QStringLiteral("rc");
    case ReleaseChannel::Stable:
        return QStringLiteral("stable");
    case ReleaseChannel::Beta:
        return QStringLiteral("beta");
    }
    return QStringLiteral("beta");
}

ReleaseChannel FeaturePolicy::defaultReleaseChannelFromBuild()
{
    return releaseChannelFromString(BuildInfo::buildChannel());
}

bool FeaturePolicy::defaultShowExperimentalFeatures(ReleaseChannel channel)
{
    switch (channel) {
    case ReleaseChannel::Stable:
    case ReleaseChannel::Rc:
        return false;
    case ReleaseChannel::Beta:
    case ReleaseChannel::Dev:
        return true;
    }
    return true;
}

bool FeaturePolicy::isStableLikeChannel(ReleaseChannel channel)
{
    return channel == ReleaseChannel::Stable || channel == ReleaseChannel::Rc;
}

bool FeaturePolicy::isExperimentalFeature(FeatureId id)
{
    switch (id) {
    case FeatureId::XraySystemProxyRuntime:
    case FeatureId::CoreUpdateManager:
    case FeatureId::BackupImport:
    case FeatureId::DiagnosticsBundle:
        return false;
    case FeatureId::SingBoxTunExperimental:
    case FeatureId::ZaryaHelper:
    case FeatureId::KillSwitchLinuxNft:
    case FeatureId::KillSwitchWindowsWfp:
    case FeatureId::HelperServiceManagement:
    case FeatureId::AppSelfUpdate:
        return true;
    }
    return true;
}

} // namespace zarya
