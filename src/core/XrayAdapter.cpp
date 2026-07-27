#include "core/XrayAdapter.h"

#include "core/XrayConfigBuilder.h"
#include "core/XrayStreamSettings.h"
#include "dns/XrayDnsGenerator.h"
#include "routing/XrayRoutingGenerator.h"
#include "storage/AppSettings.h"

#include <QJsonArray>
#include <QRegularExpression>

namespace zarya {

namespace {

bool supportsVless(const Profile& profile, QString* reason)
{
    if (!XrayStreamSettings::isNetworkSupported(profile, reason)) {
        return false;
    }
    const QString network = XrayStreamSettings::normalizedNetwork(profile);
    if (profile.isSecurityReality()) {
        if (network != QStringLiteral("tcp")) {
            if (reason) {
                *reason = QStringLiteral("VLESS REALITY requires network tcp.");
            }
            return false;
        }
        if (profile.publicKey.trimmed().isEmpty() || profile.effectiveServerName().isEmpty()) {
            if (reason) {
                *reason = QStringLiteral("VLESS REALITY requires public key and server name.");
            }
            return false;
        }
        return true;
    }
    if (profile.isSecurityTls()) {
        return true;
    }
    if (profile.isSecurityNone()) {
        return network == QStringLiteral("tcp");
    }
    if (reason) {
        *reason = QStringLiteral("Unsupported VLESS security: %1").arg(profile.security);
    }
    return false;
}

bool supportsVmess(const Profile& profile, QString* reason)
{
    if (profile.isSecurityReality()) {
        if (reason) {
            *reason = QStringLiteral("VMess does not support REALITY security.");
        }
        return false;
    }
    if (!XrayStreamSettings::isNetworkSupported(profile, reason)) {
        return false;
    }
    if (profile.effectiveUuid().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("VMess requires UUID.");
        }
        return false;
    }
    if (profile.alterId < 0) {
        if (reason) {
            *reason = QStringLiteral("VMess alterId must be >= 0.");
        }
        return false;
    }
    const QString network = XrayStreamSettings::normalizedNetwork(profile);
    if (network == QStringLiteral("ws") && profile.path.trimmed().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("VMess WebSocket requires path.");
        }
        return false;
    }
    if (network == QStringLiteral("grpc") && profile.serviceName.trimmed().isEmpty()
        && profile.path.trimmed().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("VMess gRPC requires service name.");
        }
        return false;
    }
    return true;
}

bool supportsTrojan(const Profile& profile, QString* reason)
{
    if (profile.effectivePassword().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("Trojan requires password.");
        }
        return false;
    }
    if (!XrayStreamSettings::isNetworkSupported(profile, reason)) {
        return false;
    }
    const QString network = XrayStreamSettings::normalizedNetwork(profile);
    if (profile.isSecurityReality()) {
        if (network != QStringLiteral("tcp")) {
            if (reason) {
                *reason = QStringLiteral("Trojan REALITY requires network tcp.");
            }
            return false;
        }
        if (profile.publicKey.trimmed().isEmpty()) {
            if (reason) {
                *reason = QStringLiteral("Trojan REALITY requires public key.");
            }
            return false;
        }
        return true;
    }
    if (network == QStringLiteral("ws") && profile.path.trimmed().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("Trojan WebSocket requires path.");
        }
        return false;
    }
    return true;
}

bool supportsShadowsocks(const Profile& profile, QString* reason)
{
    if (profile.hasUnsupportedFeature()) {
        if (reason) {
            *reason = profile.unsupportedReason;
        }
        return false;
    }
    if (profile.effectiveMethod().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("Shadowsocks requires method.");
        }
        return false;
    }
    if (profile.effectivePassword().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("Shadowsocks requires password.");
        }
        return false;
    }
    return true;
}

bool supportsSocks(const Profile& profile, QString* reason)
{
    if (profile.address.trimmed().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("SOCKS requires address.");
        }
        return false;
    }
    return true;
}

bool supportsHysteria2(const Profile& profile, QString* reason)
{
    if (profile.hasUnsupportedFeature()) {
        if (reason) {
            *reason = profile.unsupportedReason;
        }
        return false;
    }
    if (profile.effectivePassword().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("Hysteria2 requires password/auth.");
        }
        return false;
    }
    if (!profile.isSecurityTls() && !profile.security.trimmed().isEmpty()
        && profile.security.trimmed().compare(QStringLiteral("none"), Qt::CaseInsensitive) != 0) {
        // Hy2 typically uses TLS; allow empty (treated as tls in generator) or explicit tls.
        if (reason) {
            *reason = QStringLiteral("Hysteria2 currently supports TLS security only.");
        }
        return false;
    }
    return true;
}

bool supportsWireGuard(const Profile& profile, QString* reason)
{
    if (profile.hasUnsupportedFeature()) {
        if (reason) {
            *reason = profile.unsupportedReason;
        }
        return false;
    }
    if (profile.effectivePassword().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("WireGuard requires a private key.");
        }
        return false;
    }
    if (profile.publicKey.trimmed().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("WireGuard requires a peer public key.");
        }
        return false;
    }
    return true;
}

QStringList splitCsvList(const QString& value)
{
    QStringList parts;
    for (const QString& part : value.split(QRegularExpression(QStringLiteral("[,\\s]+")),
                                            Qt::SkipEmptyParts)) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            parts.append(trimmed);
        }
    }
    return parts;
}

QJsonArray parseReservedBytes(const QString& reserved)
{
    const QString trimmed = reserved.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    // Comma/space-separated decimal bytes: "1,2,3"
    if (trimmed.contains(QLatin1Char(',')) || trimmed.contains(QLatin1Char(' '))) {
        QJsonArray bytes;
        for (const QString& part : splitCsvList(trimmed)) {
            bool ok = false;
            const int value = part.toInt(&ok);
            if (!ok || value < 0 || value > 255) {
                return {};
            }
            bytes.append(value);
        }
        return bytes.size() == 3 ? bytes : QJsonArray{};
    }

    // Single decimal triplet without separators is uncommon; try base64 (3 bytes).
    QByteArray decoded = QByteArray::fromBase64(trimmed.toUtf8());
    if (decoded.size() != 3) {
        QString normalized = trimmed;
        const int remainder = normalized.size() % 4;
        if (remainder != 0) {
            normalized.append(QString(4 - remainder, QLatin1Char('=')));
        }
        decoded = QByteArray::fromBase64(normalized.toUtf8());
    }
    if (decoded.size() != 3) {
        return {};
    }
    return QJsonArray{static_cast<int>(static_cast<uchar>(decoded.at(0))),
                      static_cast<int>(static_cast<uchar>(decoded.at(1))),
                      static_cast<int>(static_cast<uchar>(decoded.at(2)))};
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
    ports.mixedPort = settings.mixedPort();
    return generateConfigInternal(profile, ports, nullptr, nullptr);
}

ConfigGenerationResult XrayAdapter::generateConfig(const Profile& profile,
                                                   const XrayInboundPorts& ports) const
{
    return generateConfigInternal(profile, ports, nullptr, nullptr);
}

ConfigGenerationResult XrayAdapter::generateConfig(const Profile& profile,
                                                   const XrayInboundPorts& ports,
                                                   const RoutingProfile& routingProfile) const
{
    return generateConfigInternal(profile, ports, &routingProfile, nullptr);
}

ConfigGenerationResult XrayAdapter::generateConfig(const Profile& profile,
                                                   const XrayInboundPorts& ports,
                                                   const RoutingProfile& routingProfile,
                                                   const DnsProfile& dnsProfile) const
{
    return generateConfigInternal(profile, ports, &routingProfile, &dnsProfile);
}

ConfigGenerationResult XrayAdapter::generateConfigInternal(const Profile& profile,
                                                           const XrayInboundPorts& ports,
                                                           const RoutingProfile* routingProfile,
                                                           const DnsProfile* dnsProfile) const
{
    QString reason;
    if (!supportsProfile(profile, &reason)) {
        return {false, {}, reason};
    }

    QString error;
    const QJsonObject proxyOutbound = generateOutbound(profile, &error);
    if (!error.isEmpty() || proxyOutbound.isEmpty()) {
        return {false, {}, error.isEmpty() ? QStringLiteral("Failed to generate outbound.")
                                           : error};
    }

    QJsonObject routing;
    if (routingProfile) {
        const XrayRoutingGenerator routingGenerator;
        routing = routingGenerator.generate(*routingProfile);
    }

    QJsonObject dns;
    if (dnsProfile) {
        const XrayDnsGenerator dnsGenerator;
        if (dnsGenerator.shouldGenerateDnsObject(*dnsProfile)) {
            dns = dnsGenerator.generate(*dnsProfile);
        }
    }

    if (!routing.isEmpty() || !dns.isEmpty()) {
        return {true, XrayConfigBuilder::buildFullConfig(proxyOutbound, ports, routing, dns), {}};
    }

    return {true, XrayConfigBuilder::buildFullConfig(proxyOutbound, ports), {}};
}

QJsonObject XrayAdapter::wrapProxyOutbound(const QString& protocol, const QJsonObject& settings,
                                           const QJsonObject& streamSettings)
{
    QJsonObject outbound;
    outbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    outbound.insert(QStringLiteral("protocol"), protocol);
    outbound.insert(QStringLiteral("settings"), settings);
    if (!streamSettings.isEmpty()) {
        outbound.insert(QStringLiteral("streamSettings"), streamSettings);
    }
    return outbound;
}

QJsonObject XrayAdapter::generateOutbound(const Profile& profile, QString* errorMessage) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        return generateVlessOutbound(profile, errorMessage);
    case ProtocolType::Vmess:
        return generateVmessOutbound(profile, errorMessage);
    case ProtocolType::Trojan:
        return generateTrojanOutbound(profile, errorMessage);
    case ProtocolType::Shadowsocks:
        return generateShadowsocksOutbound(profile, errorMessage);
    case ProtocolType::Socks:
        return generateSocksOutbound(profile, errorMessage);
    case ProtocolType::Hysteria2:
        return generateHysteria2Outbound(profile, errorMessage);
    case ProtocolType::WireGuard:
        return generateWireGuardOutbound(profile, errorMessage);
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Unknown protocol.");
    }
    return {};
}

QJsonObject XrayAdapter::generateVlessOutbound(const Profile& profile,
                                                 QString* errorMessage) const
{
    QJsonObject user;
    user.insert(QStringLiteral("id"), profile.effectiveUuid());
    user.insert(QStringLiteral("encryption"), profile.effectiveEncryption());
    const QString flow = profile.flow.trimmed();
    if (!flow.isEmpty()) {
        user.insert(QStringLiteral("flow"), flow);
    }

    QJsonObject vnextEntry;
    vnextEntry.insert(QStringLiteral("address"), profile.address.trimmed());
    vnextEntry.insert(QStringLiteral("port"), profile.port);
    vnextEntry.insert(QStringLiteral("users"), QJsonArray{user});

    QJsonObject settings;
    settings.insert(QStringLiteral("vnext"), QJsonArray{vnextEntry});

    QString streamError;
    const QJsonObject streamSettings = XrayStreamSettings::generate(profile, &streamError);
    if (!streamError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = streamError;
        }
        return {};
    }

    return wrapProxyOutbound(QStringLiteral("vless"), settings, streamSettings);
}

QJsonObject XrayAdapter::generateVmessOutbound(const Profile& profile,
                                               QString* errorMessage) const
{
    QJsonObject user;
    user.insert(QStringLiteral("id"), profile.effectiveUuid());
    user.insert(QStringLiteral("alterId"), profile.alterId);
    user.insert(QStringLiteral("security"), profile.effectiveVmessSecurity());

    QJsonObject vnextEntry;
    vnextEntry.insert(QStringLiteral("address"), profile.address.trimmed());
    vnextEntry.insert(QStringLiteral("port"), profile.port);
    vnextEntry.insert(QStringLiteral("users"), QJsonArray{user});

    QJsonObject settings;
    settings.insert(QStringLiteral("vnext"), QJsonArray{vnextEntry});

    QString streamError;
    const QJsonObject streamSettings = XrayStreamSettings::generate(profile, &streamError);
    if (!streamError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = streamError;
        }
        return {};
    }

    return wrapProxyOutbound(QStringLiteral("vmess"), settings, streamSettings);
}

QJsonObject XrayAdapter::generateTrojanOutbound(const Profile& profile,
                                                QString* errorMessage) const
{
    QJsonObject server;
    server.insert(QStringLiteral("address"), profile.address.trimmed());
    server.insert(QStringLiteral("port"), profile.port);
    server.insert(QStringLiteral("password"), profile.effectivePassword());

    QJsonObject settings;
    settings.insert(QStringLiteral("servers"), QJsonArray{server});

    QString streamError;
    const QJsonObject streamSettings = XrayStreamSettings::generate(profile, &streamError);
    if (!streamError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = streamError;
        }
        return {};
    }

    return wrapProxyOutbound(QStringLiteral("trojan"), settings, streamSettings);
}

QJsonObject XrayAdapter::generateShadowsocksOutbound(const Profile& profile,
                                                     QString* errorMessage) const
{
    QJsonObject server;
    server.insert(QStringLiteral("address"), profile.address.trimmed());
    server.insert(QStringLiteral("port"), profile.port);
    server.insert(QStringLiteral("method"), profile.effectiveMethod());
    server.insert(QStringLiteral("password"), profile.effectivePassword());

    QJsonObject settings;
    settings.insert(QStringLiteral("servers"), QJsonArray{server});

    return wrapProxyOutbound(QStringLiteral("shadowsocks"), settings, {});
}

QJsonObject XrayAdapter::generateSocksOutbound(const Profile& profile,
                                               QString* errorMessage) const
{
    QJsonObject user;
    const QString pass = profile.effectivePassword();
    if (!pass.isEmpty()) {
        user.insert(QStringLiteral("pass"), pass);
    }

    QJsonObject server;
    server.insert(QStringLiteral("address"), profile.address.trimmed());
    server.insert(QStringLiteral("port"), profile.port);
    if (!user.isEmpty()) {
        server.insert(QStringLiteral("users"), QJsonArray{user});
    }

    QJsonObject settings;
    settings.insert(QStringLiteral("servers"), QJsonArray{server});

    return wrapProxyOutbound(QStringLiteral("socks"), settings, {});
}

QJsonObject XrayAdapter::generateHysteria2Outbound(const Profile& profile,
                                                   QString* errorMessage) const
{
    QJsonObject settings;
    settings.insert(QStringLiteral("version"), 2);
    settings.insert(QStringLiteral("address"), profile.address.trimmed());
    settings.insert(QStringLiteral("port"), profile.port);

    QJsonObject hysteriaSettings;
    hysteriaSettings.insert(QStringLiteral("version"), 2);
    hysteriaSettings.insert(QStringLiteral("auth"), profile.effectivePassword());

    QJsonObject streamSettings;
    streamSettings.insert(QStringLiteral("network"), QStringLiteral("hysteria"));
    streamSettings.insert(QStringLiteral("security"), QStringLiteral("tls"));

    Profile tlsProfile = profile;
    if (tlsProfile.security.trimmed().isEmpty()) {
        tlsProfile.security = QStringLiteral("tls");
    }
    if (tlsProfile.alpn.trimmed().isEmpty()) {
        tlsProfile.alpn = QStringLiteral("h3");
    }
    streamSettings.insert(QStringLiteral("tlsSettings"),
                          XrayStreamSettings::buildTlsSettings(tlsProfile));
    streamSettings.insert(QStringLiteral("hysteriaSettings"), hysteriaSettings);

    const QString obfs = profile.obfs.trimmed().toLower();
    if (obfs == QStringLiteral("salamander")) {
        const QString obfsPassword = profile.obfsPassword.trimmed();
        if (obfsPassword.isEmpty()) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("Hysteria2 salamander obfuscation requires obfs-password.");
            }
            return {};
        }
        QJsonObject maskSettings;
        maskSettings.insert(QStringLiteral("password"), obfsPassword);
        QJsonObject mask;
        mask.insert(QStringLiteral("type"), QStringLiteral("salamander"));
        mask.insert(QStringLiteral("settings"), maskSettings);
        streamSettings.insert(QStringLiteral("finalmask"),
                              QJsonObject{{QStringLiteral("udp"), QJsonArray{mask}}});
    }

    return wrapProxyOutbound(QStringLiteral("hysteria"), settings, streamSettings);
}

QJsonObject XrayAdapter::generateWireGuardOutbound(const Profile& profile,
                                                   QString* errorMessage) const
{
    const QString secretKey = profile.effectivePassword();
    const QString peerPublicKey = profile.publicKey.trimmed();
    if (secretKey.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("WireGuard private key is required.");
        }
        return {};
    }
    if (peerPublicKey.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("WireGuard peer public key is required.");
        }
        return {};
    }

    QJsonObject peer;
    peer.insert(QStringLiteral("endpoint"),
                QStringLiteral("%1:%2").arg(profile.address.trimmed()).arg(profile.port));
    peer.insert(QStringLiteral("publicKey"), peerPublicKey);
    if (!profile.preSharedKey.trimmed().isEmpty()) {
        peer.insert(QStringLiteral("preSharedKey"), profile.preSharedKey.trimmed());
    }
    if (profile.keepAlive > 0) {
        peer.insert(QStringLiteral("keepAlive"), profile.keepAlive);
    }
    const QStringList allowedIps = splitCsvList(profile.allowedIps);
    if (!allowedIps.isEmpty()) {
        peer.insert(QStringLiteral("allowedIPs"), QJsonArray::fromStringList(allowedIps));
    }

    QJsonObject settings;
    settings.insert(QStringLiteral("secretKey"), secretKey);
    settings.insert(QStringLiteral("peers"), QJsonArray{peer});
    // Prefer userspace path so system-proxy mode does not require kernel TUN privileges.
    settings.insert(QStringLiteral("noKernelTun"), true);

    const QStringList localAddresses = splitCsvList(profile.localAddress);
    if (!localAddresses.isEmpty()) {
        settings.insert(QStringLiteral("address"), QJsonArray::fromStringList(localAddresses));
    }
    if (profile.mtu > 0) {
        settings.insert(QStringLiteral("mtu"), profile.mtu);
    }

    const QJsonArray reserved = parseReservedBytes(profile.reserved);
    if (!reserved.isEmpty()) {
        settings.insert(QStringLiteral("reserved"), reserved);
    } else if (!profile.reserved.trimmed().isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("WireGuard reserved must be 3 bytes (e.g. 1,2,3 or base64).");
        return {};
    }

    return wrapProxyOutbound(QStringLiteral("wireguard"), settings, {});
}

bool XrayAdapter::supportsProfile(const Profile& profile, QString* reason) const
{
    if (!profile.isValid()) {
        if (reason) {
            *reason = QStringLiteral("Profile name, address, and port are required.");
        }
        return false;
    }

    if (profile.hasUnsupportedFeature()) {
        if (reason) {
            *reason = profile.unsupportedReason;
        }
        return false;
    }

    switch (profile.protocol) {
    case ProtocolType::Vless:
        return supportsVless(profile, reason);
    case ProtocolType::Vmess:
        return supportsVmess(profile, reason);
    case ProtocolType::Trojan:
        return supportsTrojan(profile, reason);
    case ProtocolType::Shadowsocks:
        return supportsShadowsocks(profile, reason);
    case ProtocolType::Socks:
        return supportsSocks(profile, reason);
    case ProtocolType::Hysteria2:
        return supportsHysteria2(profile, reason);
    case ProtocolType::WireGuard:
        return supportsWireGuard(profile, reason);
    }

    if (reason) {
        *reason = QStringLiteral("Unsupported protocol.");
    }
    return false;
}

QStringList XrayAdapter::argumentsForConfig(const QString& configPath) const
{
    return {QStringLiteral("run"), QStringLiteral("-config"), configPath};
}

} // namespace zarya
