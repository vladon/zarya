#include "updater/AppUpdatePolicy.h"

#include "packaging/InstallationMode.h"
#include "updater/AppVersion.h"

namespace zarya {

QStringList AppUpdatePolicy::buildWarnings(const AppUpdateChannelEntry& entry,
                                           const AppUpdateAsset& asset,
                                           bool allowUnsignedDownload)
{
    QStringList warnings;
    if (asset.sha256.isEmpty()) {
        warnings.append(QStringLiteral(
            "This update asset has no checksum/signature. Zarya will not install it automatically."));
        if (!allowUnsignedDownload) {
            warnings.append(QStringLiteral("Download and verify is blocked unless unsigned app "
                                           "update download is enabled in Settings."));
        }
    } else if (asset.signature.type != QStringLiteral("none") && !asset.signature.type.isEmpty()) {
        warnings.append(QStringLiteral(
            "Signature verification is not implemented for this signature type."));
    }

    const InstallationMode mode = InstallationInfo::currentMode();
    const bool wantsInstalled = mode == InstallationMode::Installed;
    const bool assetPortable = asset.installationMode == QStringLiteral("portable");
    const bool assetInstalled = asset.installationMode == QStringLiteral("installed");

    if (wantsInstalled && assetPortable && !assetInstalled) {
        warnings.append(QStringLiteral(
            "Only a portable artifact is available, but this app appears to be installed. "
            "Automatic update is disabled."));
    }

    return warnings;
}

QStringList AppUpdatePolicy::buildBlockers(const AppUpdateChannelEntry& entry,
                                           const QString& currentVersion,
                                           AppUpdateChannel userChannel,
                                           const QString& selectedChannelKey,
                                           bool hasMatchingAsset)
{
    QStringList blockers;

    if (!AppUpdateChannelPolicy::canSeeChannel(userChannel, selectedChannelKey)) {
        blockers.append(QStringLiteral(
            "The manifest contains a newer build on a channel that is not available for your "
            "selected app update channel."));
    }

    if (!entry.minSupportedVersion.isEmpty()
        && AppVersion::isLessThan(currentVersion, entry.minSupportedVersion)) {
        blockers.append(QStringLiteral(
            "Your version is older than the minimum supported update path. Download and install "
            "manually."));
    }

    if (!hasMatchingAsset) {
        blockers.append(QStringLiteral("No update asset matches this platform, architecture, and "
                                       "installation mode."));
    }

    return blockers;
}

} // namespace zarya
