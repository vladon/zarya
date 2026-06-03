#include "core/XrayAdapter.h"

#include "core/XrayVlessGenerator.h"
#include "domain/ProfileValidation.h"
#include "storage/AppSettings.h"

namespace zarya {

namespace {

bool equalsIgnoreCase(const QString& a, const QString& b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

} // namespace

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
    XrayInboundPorts ports;
    const AppSettings& settings = AppSettings::instance();
    ports.socksPort = settings.socksPort();
    ports.httpPort = settings.httpPort();
    return generateConfigInternal(profile, ports);
}

ConfigGenerationResult XrayAdapter::generateConfig(const Profile& profile,
                                                   const XrayInboundPorts& ports) const
{
    return generateConfigInternal(profile, ports);
}

ConfigGenerationResult XrayAdapter::generateConfigInternal(const Profile& profile,
                                                          const XrayInboundPorts& ports) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        return XrayVlessGenerator::generate(profile, ports);
    case ProtocolType::Vmess:
    case ProtocolType::Trojan:
    case ProtocolType::Shadowsocks:
    case ProtocolType::Socks:
        return {false, {}, QStringLiteral("Protocol %1 is not implemented for Xray yet.")
                              .arg(protocolTypeToString(profile.protocol))};
    }
    return {false, {}, QStringLiteral("Unknown protocol.")};
}

bool XrayAdapter::supportsProfile(const Profile& profile, QString* reason) const
{
    if (profile.protocol != ProtocolType::Vless) {
        if (reason) {
            *reason = QStringLiteral("Protocol %1 is not supported for real delay test yet.")
                          .arg(protocolTypeToString(profile.protocol));
        }
        return false;
    }

    const QString network =
        profile.network.trimmed().isEmpty() ? QStringLiteral("tcp") : profile.network.trimmed();
    if (!equalsIgnoreCase(network, QStringLiteral("tcp"))) {
        if (reason) {
            *reason = QStringLiteral("Only TCP network is supported for real delay test.");
        }
        return false;
    }

    if (!profile.isSecurityReality()) {
        if (reason) {
            *reason = QStringLiteral("Only VLESS REALITY is supported for real delay test in this "
                                    "milestone.");
        }
        return false;
    }

    const ProfileValidationResult validation = validateProfileForXray(profile);
    if (!validation.ok) {
        if (reason) {
            *reason = validation.message;
        }
        return false;
    }

    return true;
}

QStringList XrayAdapter::argumentsForConfig(const QString& configPath) const
{
    return {QStringLiteral("run"), QStringLiteral("-config"), configPath};
}

} // namespace zarya
