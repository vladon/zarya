#include "core/XrayAdapter.h"

#include "core/XrayVlessGenerator.h"

namespace zarya {

CoreType XrayAdapter::type() const
{
    return CoreType::Xray;
}

QString XrayAdapter::displayName() const
{
    return QStringLiteral("Xray");
}

ConfigGenerationResult XrayAdapter::generateConfig(const Profile& profile) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        return XrayVlessGenerator::generate(profile);
    case ProtocolType::Vmess:
    case ProtocolType::Trojan:
    case ProtocolType::Shadowsocks:
    case ProtocolType::Socks:
        return {false, {}, QStringLiteral("Protocol %1 is not implemented for Xray yet.")
                              .arg(protocolTypeToString(profile.protocol))};
    }
    return {false, {}, QStringLiteral("Unknown protocol.")};
}

QStringList XrayAdapter::argumentsForConfig(const QString& configPath) const
{
    return {QStringLiteral("run"), QStringLiteral("-c"), configPath};
}

} // namespace zarya
