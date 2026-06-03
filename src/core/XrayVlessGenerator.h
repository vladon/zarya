#pragma once

#include "core/ICoreAdapter.h"
#include "domain/Profile.h"

namespace zarya {

struct XrayInboundPorts {
    int socksPort = 10808;
    int httpPort = 10809;
};

class XrayVlessGenerator {
public:
    static ConfigGenerationResult generate(const Profile& profile,
                                           const XrayInboundPorts& ports = {});
    static QJsonObject buildProxyOutbound(const Profile& profile, QString* errorMessage);
    static QJsonObject buildFullConfig(const QJsonObject& proxyOutbound,
                                       const XrayInboundPorts& ports);

private:
    static QJsonObject buildVlessUser(const Profile& profile);
    static QJsonObject buildStreamSettings(const Profile& profile, QString* errorMessage);
    static QJsonObject buildRealitySettings(const Profile& profile);
    static QJsonObject buildTlsSettings(const Profile& profile);
};

} // namespace zarya
