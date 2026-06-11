#pragma once

#include "updater/AppUpdateAsset.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace zarya {

struct AppUpdateChannelEntry {
    QString channelKey;
    QString latestVersion;
    QString minSupportedVersion;
    QString releaseNotesUrl;
    bool mandatory = false;
    QVector<AppUpdateAsset> assets;
};

struct AppUpdateManifest {
    QString format;
    int formatVersion = 0;
    QString generatedAt;
    QVector<AppUpdateChannelEntry> channels;

    bool isValid(QString* errorMessage = nullptr) const;
    static AppUpdateManifest fromJson(const QJsonObject& object, QString* errorMessage = nullptr);
    const AppUpdateChannelEntry* channelEntry(const QString& channelKey) const;
};

} // namespace zarya
