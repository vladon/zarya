#include "core/XrayAdapter.h"

#include "core/XrayConfigBuilder.h"
#include "core/XrayStreamSettings.h"
#include "dns/XrayDnsGenerator.h"
#include "routing/XrayRoutingGenerator.h"
#include "storage/AppSettings.h"

#include <QJsonArray>

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
