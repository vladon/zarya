#include "killswitch/KillSwitchPayloadBuilder.h"

#include "killswitch/KillSwitchMode.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QJsonArray>

namespace zarya {

KillSwitchPayloadResult KillSwitchPayloadBuilder::build(const Profile& profile, bool allowLan,
                                                        bool allowLoopback, bool blockDirectDns)
{
    KillSwitchPayloadResult result;
    result.rules.mode = KillSwitchMode::TunOnlyExperimental;
    result.rules.tunInterfaceName = QStringLiteral("zarya-tun");
    result.rules.proxyServerHost = profile.address.trimmed();
    result.rules.proxyServerPort = profile.port > 0 ? profile.port : 443;
    result.rules.allowLan = allowLan;
    result.rules.allowLoopback = allowLoopback;
    result.rules.blockDirectDns = blockDirectDns;

    if (result.rules.proxyServerHost.isEmpty()) {
        result.resolutionFailed = true;
        result.resolveWarning =
            QStringLiteral("Profile has no server address; cannot resolve proxy IPs.");
        return result;
    }

    QHostAddress literal(result.rules.proxyServerHost);
    if (!literal.isNull()) {
        if (literal.protocol() == QAbstractSocket::IPv4Protocol) {
            result.rules.proxyServerIpv4.append(literal.toString());
        } else if (literal.protocol() == QAbstractSocket::IPv6Protocol) {
            result.rules.proxyServerIpv6.append(literal.toString());
        }
        return result;
    }

    const QHostInfo hostInfo = QHostInfo::fromName(result.rules.proxyServerHost);
    if (hostInfo.error() != QHostInfo::NoError) {
        result.resolutionFailed = true;
        result.resolveWarning = QStringLiteral("Could not resolve %1: %2")
                                    .arg(result.rules.proxyServerHost, hostInfo.errorString());
        return result;
    }

    for (const QHostAddress& address : hostInfo.addresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            result.rules.proxyServerIpv4.append(address.toString());
        } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
            result.rules.proxyServerIpv6.append(address.toString());
        }
    }

    if (result.rules.proxyServerIpv4.isEmpty() && result.rules.proxyServerIpv6.isEmpty()) {
        result.resolutionFailed = true;
        result.resolveWarning =
            QStringLiteral("No IP addresses returned for %1.").arg(result.rules.proxyServerHost);
    }

    return result;
}

QJsonObject KillSwitchPayloadBuilder::toJson(const KillSwitchRuleSet& rules)
{
    QJsonArray ipv4;
    for (const QString& ip : rules.proxyServerIpv4) {
        ipv4.append(ip);
    }
    QJsonArray ipv6;
    for (const QString& ip : rules.proxyServerIpv6) {
        ipv6.append(ip);
    }
    return QJsonObject{
        {QStringLiteral("mode"), killSwitchModeToString(rules.mode)},
        {QStringLiteral("tunInterfaceName"), rules.tunInterfaceName},
        {QStringLiteral("proxyServerHost"), rules.proxyServerHost},
        {QStringLiteral("proxyServerIps"), ipv4},
        {QStringLiteral("proxyServerIpv4"), ipv4},
        {QStringLiteral("proxyServerIpv6"), ipv6},
        {QStringLiteral("proxyServerPort"), rules.proxyServerPort},
        {QStringLiteral("allowLan"), rules.allowLan},
        {QStringLiteral("allowLoopback"), rules.allowLoopback},
        {QStringLiteral("blockDirectDns"), rules.blockDirectDns},
    };
}

} // namespace zarya
