#include "updater/AppUpdateChannel.h"

#include "app/BuildInfo.h"

namespace zarya {

AppUpdateChannel AppUpdateChannelPolicy::fromString(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower == QStringLiteral("dev")) {
        return AppUpdateChannel::Dev;
    }
    if (lower == QStringLiteral("stable")) {
        return AppUpdateChannel::Stable;
    }
    return AppUpdateChannel::Beta;
}

QString AppUpdateChannelPolicy::toString(AppUpdateChannel channel)
{
    switch (channel) {
    case AppUpdateChannel::Dev:
        return QStringLiteral("dev");
    case AppUpdateChannel::Stable:
        return QStringLiteral("stable");
    case AppUpdateChannel::Beta:
        return QStringLiteral("beta");
    }
    return QStringLiteral("beta");
}

AppUpdateChannel AppUpdateChannelPolicy::defaultChannelFromBuild()
{
    return fromString(BuildInfo::buildChannel());
}

QStringList AppUpdateChannelPolicy::allowedManifestChannelKeys(AppUpdateChannel channel)
{
    switch (channel) {
    case AppUpdateChannel::Dev:
        return {QStringLiteral("dev"), QStringLiteral("beta"), QStringLiteral("stable")};
    case AppUpdateChannel::Beta:
        return {QStringLiteral("beta"), QStringLiteral("stable")};
    case AppUpdateChannel::Stable:
        return {QStringLiteral("stable")};
    }
    return {QStringLiteral("beta")};
}

bool AppUpdateChannelPolicy::canSeeChannel(AppUpdateChannel userChannel,
                                           const QString& manifestChannelKey)
{
    return allowedManifestChannelKeys(userChannel).contains(manifestChannelKey,
                                                            Qt::CaseInsensitive);
}

} // namespace zarya
