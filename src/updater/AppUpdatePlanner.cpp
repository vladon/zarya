#include "updater/AppUpdatePlanner.h"

#include "packaging/InstallationMode.h"
#include "storage/AppSettings.h"
#include "updater/AppUpdatePolicy.h"
#include "updater/AppVersion.h"

#include <QSysInfo>

#include <optional>

namespace zarya {

namespace {

QString currentPlatformToken()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return QStringLiteral("unknown");
#endif
}

QString currentArchitectureToken()
{
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    if (arch.contains(QStringLiteral("arm64")) || arch.contains(QStringLiteral("aarch64"))) {
        return QStringLiteral("arm64");
    }
    return QStringLiteral("x64");
}

bool architectureMatches(const QString& manifestArch, const QString& currentArch)
{
    const QString lower = manifestArch.toLower();
    if (lower == currentArch) {
        return true;
    }
    if ((lower == QStringLiteral("x64") || lower == QStringLiteral("amd64"))
        && (currentArch == QStringLiteral("x64") || currentArch == QStringLiteral("amd64"))) {
        return true;
    }
    if ((lower == QStringLiteral("macos") || lower == QStringLiteral("darwin"))
        && currentPlatformToken() == QStringLiteral("macos")) {
        return true;
    }
    return false;
}

bool platformMatches(const QString& manifestPlatform)
{
    const QString lower = manifestPlatform.toLower();
    const QString current = currentPlatformToken();
    if (lower == current) {
        return true;
    }
    if (lower == QStringLiteral("darwin") && current == QStringLiteral("macos")) {
        return true;
    }
    return false;
}

QString preferredInstallationMode()
{
    switch (InstallationInfo::currentMode()) {
    case InstallationMode::Installed:
        return QStringLiteral("installed");
    case InstallationMode::Portable:
        return QStringLiteral("portable");
    case InstallationMode::Unknown:
        return QStringLiteral("portable");
    }
    return QStringLiteral("portable");
}

int installationModeScore(const AppUpdateAsset& asset, const QString& preferredMode)
{
    if (asset.installationMode == preferredMode) {
        return 2;
    }
    if (preferredMode == QStringLiteral("installed")
        && asset.installationMode == QStringLiteral("portable")) {
        return 0;
    }
    if (preferredMode == QStringLiteral("portable")
        && asset.installationMode == QStringLiteral("installed")) {
        return 1;
    }
    return 1;
}

std::optional<AppUpdateAsset> selectAsset(const QVector<AppUpdateAsset>& assets)
{
    const QString currentArch = currentArchitectureToken();
    const QString preferredMode = preferredInstallationMode();

    const AppUpdateAsset* best = nullptr;
    int bestScore = -1;
    for (const AppUpdateAsset& asset : assets) {
        if (!platformMatches(asset.platform)) {
            continue;
        }
        if (!architectureMatches(asset.architecture, currentArch)) {
            continue;
        }
        const int score = installationModeScore(asset, preferredMode);
        if (score > bestScore) {
            bestScore = score;
            best = &asset;
        }
    }
    if (best == nullptr) {
        return std::nullopt;
    }
    return *best;
}

const AppUpdateChannelEntry* selectBestChannelEntry(const AppUpdateManifest& manifest,
                                                    const QString& currentVersion,
                                                    AppUpdateChannel userChannel,
                                                    QString* selectedChannelKey)
{
    const QStringList allowedKeys = AppUpdateChannelPolicy::allowedManifestChannelKeys(userChannel);
    const AppUpdateChannelEntry* bestEntry = nullptr;
    QString bestKey;
    for (const QString& key : allowedKeys) {
        const AppUpdateChannelEntry* entry = manifest.channelEntry(key);
        if (entry == nullptr || entry->latestVersion.isEmpty()) {
            continue;
        }
        if (AppVersion::compare(entry->latestVersion, currentVersion) <= 0) {
            continue;
        }
        if (bestEntry == nullptr
            || AppVersion::isGreaterThan(entry->latestVersion, bestEntry->latestVersion)) {
            bestEntry = entry;
            bestKey = key;
        }
    }
    if (selectedChannelKey != nullptr) {
        *selectedChannelKey = bestKey;
    }
    return bestEntry;
}

} // namespace

AppUpdatePlan AppUpdatePlanner::buildPlan(const AppUpdateManifest& manifest,
                                            const QString& currentVersion,
                                            AppUpdateChannel userChannel)
{
    AppUpdatePlan plan;
    plan.currentVersion = currentVersion;

    QString selectedChannelKey;
    const AppUpdateChannelEntry* entry =
        selectBestChannelEntry(manifest, currentVersion, userChannel, &selectedChannelKey);
    if (entry == nullptr) {
        plan.latestVersion = currentVersion;
        plan.updateAvailable = false;
        return plan;
    }

    plan.selectedChannelKey = selectedChannelKey;
    plan.channelEntry = *entry;
    plan.latestVersion = entry->latestVersion;

    const std::optional<AppUpdateAsset> asset = selectAsset(entry->assets);
    const bool hasAsset = asset.has_value();
    if (hasAsset) {
        plan.selectedAsset = asset.value();
    }

    plan.blockers =
        AppUpdatePolicy::buildBlockers(*entry, currentVersion, userChannel, selectedChannelKey,
                                       hasAsset && plan.selectedAsset.isValid());

    const InstallationMode mode = InstallationInfo::currentMode();
    if (mode == InstallationMode::Installed && hasAsset
        && plan.selectedAsset.installationMode == QStringLiteral("portable")) {
        plan.blockers.append(QStringLiteral(
            "Installed-mode updates require a platform installer. Automatic installation is not "
            "implemented yet."));
    }

    for (const AppUpdateChannelEntry& manifestEntry : manifest.channels) {
        if (manifestEntry.channelKey == selectedChannelKey) {
            continue;
        }
        if (!AppUpdateChannelPolicy::canSeeChannel(userChannel, manifestEntry.channelKey)) {
            if (AppVersion::isGreaterThan(manifestEntry.latestVersion, plan.latestVersion)) {
                plan.warnings.append(QStringLiteral(
                    "The manifest contains a newer %1 build, but your channel is %2.")
                                         .arg(manifestEntry.channelKey,
                                              AppUpdateChannelPolicy::toString(userChannel)));
            }
        }
    }

    if (hasAsset) {
        plan.warnings.append(AppUpdatePolicy::buildWarnings(
            *entry, plan.selectedAsset, AppSettings::instance().allowUnsignedAppUpdates()));
    }

    plan.updateAvailable = AppVersion::isGreaterThan(plan.latestVersion, currentVersion) && hasAsset
                           && plan.selectedAsset.isValid();
    return plan;
}

} // namespace zarya
