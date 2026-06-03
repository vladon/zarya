#pragma once

#include "domain/Profile.h"

#include <QJsonObject>
#include <QString>

namespace zarya {

class XrayStreamSettings {
public:
    static QString normalizedNetwork(const Profile& profile);
    static bool isNetworkSupported(const Profile& profile, QString* reason = nullptr);

    static QJsonObject generate(const Profile& profile, QString* errorMessage = nullptr);

private:
    static QJsonObject buildTlsSettings(const Profile& profile);
    static QJsonObject buildRealitySettings(const Profile& profile);
    static QJsonObject buildWsSettings(const Profile& profile);
    static QJsonObject buildGrpcSettings(const Profile& profile);
    static void applyTransportLayer(const Profile& profile, const QString& network,
                                    QJsonObject& streamSettings, QString* errorMessage);
    static void applySecurityLayer(const Profile& profile, const QString& network,
                                   QJsonObject& streamSettings, QString* errorMessage);
};

} // namespace zarya
