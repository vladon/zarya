#include "cores/CoreAssetSelector.h"

namespace zarya {

namespace {

QString lower(const QString& value)
{
    return value.toLower();
}

bool containsAny(const QString& haystack, const QStringList& needles)
{
    for (const QString& needle : needles) {
        if (haystack.contains(needle)) {
            return true;
        }
    }
    return false;
}

} // namespace

bool CoreAssetSelector::isRejectedAsset(const QString& name)
{
    const QString asset = lower(name);
    if (asset.contains(QStringLiteral("source"))) {
        return true;
    }
    if (asset.endsWith(QStringLiteral(".sha256")) || asset.endsWith(QStringLiteral(".sha256sum"))
        || asset.contains(QStringLiteral("checksum")) || asset == QStringLiteral("sha256sums")
        || asset == QStringLiteral("checksums.txt")) {
        return true;
    }
    if (asset.contains(QStringLiteral("debug")) || asset.contains(QStringLiteral("symbol"))) {
        return true;
    }
    if (!(asset.endsWith(QStringLiteral(".zip")) || asset.endsWith(QStringLiteral(".tar.gz"))
          || asset.endsWith(QStringLiteral(".tgz")))) {
        return true;
    }
    return false;
}

int CoreAssetSelector::scoreAsset(const CoreAsset& asset, CoreType type,
                                  const CorePlatformInfo& platform)
{
    if (isRejectedAsset(asset.name)) {
        return -1000;
    }

    int score = 0;
    const QString name = lower(asset.name);
    const QString coreName = type == CoreType::Xray ? QStringLiteral("xray") : QStringLiteral("sing-box");
    if (name.contains(coreName)) {
        score += 50;
    }

    if (platform.osToken == QStringLiteral("windows")) {
        if (name.contains(QStringLiteral("windows"))) {
            score += 30;
        }
    } else if (platform.osToken == QStringLiteral("darwin")) {
        if (name.contains(QStringLiteral("darwin")) || name.contains(QStringLiteral("macos"))) {
            score += 30;
        }
    } else if (platform.osToken == QStringLiteral("linux")) {
        if (name.contains(QStringLiteral("linux"))) {
            score += 30;
        }
    }

    if (platform.archToken == QStringLiteral("arm64")) {
        if (containsAny(name, {QStringLiteral("arm64"), QStringLiteral("aarch64"),
                               QStringLiteral("arm64-v8a"), QStringLiteral("arm64-v8")})) {
            score += 30;
        }
        if (name.contains(QStringLiteral("64")) && !name.contains(QStringLiteral("arm"))) {
            score -= 20;
        }
    } else {
        if (containsAny(name, {QStringLiteral("amd64"), QStringLiteral("x86_64"),
                               QStringLiteral("linux-64"), QStringLiteral("windows-64"),
                               QStringLiteral("macos-64")})) {
            score += 30;
        }
        if (name.contains(QStringLiteral("arm64")) || name.contains(QStringLiteral("aarch64"))) {
            score -= 50;
        }
    }

    if (name.endsWith(QStringLiteral(".zip")) || name.endsWith(QStringLiteral(".tar.gz"))
        || name.endsWith(QStringLiteral(".tgz"))) {
        score += 10;
    }

    return score;
}

std::optional<CoreAsset> CoreAssetSelector::selectBest(const CoreRelease& release,
                                                       const CorePlatformInfo& platform)
{
    std::optional<CoreAsset> best;
    int bestScore = -1;
    for (const CoreAsset& asset : release.assets) {
        const int score = scoreAsset(asset, release.coreType, platform);
        if (score > bestScore) {
            bestScore = score;
            best = asset;
        }
    }
    if (!best.has_value() || bestScore < 0) {
        return std::nullopt;
    }
    return best;
}

} // namespace zarya
