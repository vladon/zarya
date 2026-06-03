#pragma once

#include "core/ICoreAdapter.h"
#include "domain/Profile.h"

namespace zarya {

class XrayVlessGenerator {
public:
    static ConfigGenerationResult generate(const Profile& profile);
    static QJsonObject buildProxyOutbound(const Profile& profile, QString* errorMessage);
    static QJsonObject buildFullConfig(const QJsonObject& proxyOutbound);

private:
    static QJsonObject buildVlessUser(const Profile& profile);
    static QJsonObject buildStreamSettings(const Profile& profile, QString* errorMessage);
    static QJsonObject buildRealitySettings(const Profile& profile);
    static QJsonObject buildTlsSettings(const Profile& profile);
};

} // namespace zarya
