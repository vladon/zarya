#include "core/XrayVlessGenerator.h"

#include "core/XrayAdapter.h"
#include "core/XrayConfigBuilder.h"

namespace zarya {

ConfigGenerationResult XrayVlessGenerator::generate(const Profile& profile,
                                                     const XrayInboundPorts& ports)
{
    XrayAdapter adapter;
    return adapter.generateConfig(profile, ports);
}

QJsonObject XrayVlessGenerator::buildProxyOutbound(const Profile& profile, QString* errorMessage)
{
    XrayAdapter adapter;
    return adapter.generateOutbound(profile, errorMessage);
}

QJsonObject XrayVlessGenerator::buildFullConfig(const QJsonObject& proxyOutbound,
                                                const XrayInboundPorts& ports)
{
    return XrayConfigBuilder::buildFullConfig(proxyOutbound, ports);
}

QJsonObject XrayVlessGenerator::buildVlessUser(const Profile& profile)
{
    Q_UNUSED(profile);
    return {};
}

QJsonObject XrayVlessGenerator::buildStreamSettings(const Profile& profile,
                                                      QString* errorMessage)
{
    Q_UNUSED(profile);
    Q_UNUSED(errorMessage);
    return {};
}

QJsonObject XrayVlessGenerator::buildRealitySettings(const Profile& profile)
{
    Q_UNUSED(profile);
    return {};
}

QJsonObject XrayVlessGenerator::buildTlsSettings(const Profile& profile)
{
    Q_UNUSED(profile);
    return {};
}

} // namespace zarya
