#include "updater/AppUpdateManifest.h"

#include <QJsonArray>

namespace zarya {

bool AppUpdateManifest::isValid(QString* errorMessage) const
{
    if (format != QStringLiteral("zarya-update-manifest")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported update manifest format.");
        }
        return false;
    }
    if (formatVersion < 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported update manifest format version.");
        }
        return false;
    }
    if (channels.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update manifest contains no channels.");
        }
        return false;
    }
    return true;
}

AppUpdateManifest AppUpdateManifest::fromJson(const QJsonObject& object, QString* errorMessage)
{
    AppUpdateManifest manifest;
    manifest.format = object.value(QStringLiteral("format")).toString();
    manifest.formatVersion = object.value(QStringLiteral("formatVersion")).toInt();
    manifest.generatedAt = object.value(QStringLiteral("generatedAt")).toString();

    const QJsonObject channelsObject = object.value(QStringLiteral("channels")).toObject();
    for (auto it = channelsObject.begin(); it != channelsObject.end(); ++it) {
        const QJsonObject channelObject = it.value().toObject();
        AppUpdateChannelEntry entry;
        entry.channelKey = it.key();
        entry.latestVersion = channelObject.value(QStringLiteral("latestVersion")).toString();
        entry.minSupportedVersion =
            channelObject.value(QStringLiteral("minSupportedVersion")).toString();
        entry.releaseNotesUrl = channelObject.value(QStringLiteral("releaseNotesUrl")).toString();
        entry.mandatory = channelObject.value(QStringLiteral("mandatory")).toBool(false);

        const QJsonArray assetsArray = channelObject.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue& assetValue : assetsArray) {
            entry.assets.append(AppUpdateAsset::fromJson(assetValue.toObject()));
        }
        manifest.channels.append(entry);
    }

    if (!manifest.isValid(errorMessage)) {
        return {};
    }
    return manifest;
}

const AppUpdateChannelEntry* AppUpdateManifest::channelEntry(const QString& channelKey) const
{
    for (const AppUpdateChannelEntry& entry : channels) {
        if (entry.channelKey.compare(channelKey, Qt::CaseInsensitive) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace zarya
