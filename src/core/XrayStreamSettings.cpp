#include "core/XrayStreamSettings.h"

#include <QJsonObject>

namespace zarya {

namespace {

bool equalsIgnoreCase(const QString& a, const QString& b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

QString normalizedSecurity(const Profile& profile)
{
    return profile.security.trimmed().toLower();
}

} // namespace

QString XrayStreamSettings::normalizedNetwork(const Profile& profile)
{
    QString network = profile.network.trimmed().toLower();
    if (network.isEmpty() || network == QStringLiteral("raw")) {
        return QStringLiteral("tcp");
    }
    return network;
}

bool XrayStreamSettings::isNetworkSupported(const Profile& profile, QString* reason)
{
    const QString network = normalizedNetwork(profile);
    if (network == QStringLiteral("tcp") || network == QStringLiteral("ws")
        || network == QStringLiteral("grpc")) {
        return true;
    }
    if (reason) {
        *reason = QStringLiteral("Unsupported network type: %1").arg(profile.network);
    }
    return false;
}

QJsonObject XrayStreamSettings::buildTlsSettings(const Profile& profile)
{
    QJsonObject tlsSettings;
    QString serverName = profile.effectiveServerName();
    if (serverName.isEmpty()) {
        serverName = profile.address.trimmed();
    }
    if (!serverName.isEmpty()) {
        tlsSettings.insert(QStringLiteral("serverName"), serverName);
    }
    const QString fp = profile.effectiveFingerprint();
    if (!fp.isEmpty()) {
        tlsSettings.insert(QStringLiteral("fingerprint"), fp);
    }
    if (profile.allowInsecure) {
        tlsSettings.insert(QStringLiteral("allowInsecure"), true);
    }
    return tlsSettings;
}

QJsonObject XrayStreamSettings::buildRealitySettings(const Profile& profile)
{
    QJsonObject reality;
    reality.insert(QStringLiteral("show"), false);
    reality.insert(QStringLiteral("fingerprint"), profile.effectiveFingerprint());
    reality.insert(QStringLiteral("serverName"), profile.effectiveServerName());
    reality.insert(QStringLiteral("publicKey"), profile.publicKey.trimmed());
    reality.insert(QStringLiteral("shortId"), profile.shortId.trimmed());

    QString spiderX = profile.spiderX.trimmed();
    if (spiderX.isEmpty()) {
        spiderX = QStringLiteral("/");
    }
    reality.insert(QStringLiteral("spiderX"), spiderX);
    return reality;
}

QJsonObject XrayStreamSettings::buildWsSettings(const Profile& profile)
{
    QJsonObject wsSettings;
    QString path = profile.path.trimmed();
    if (path.isEmpty()) {
        path = QStringLiteral("/");
    }
    wsSettings.insert(QStringLiteral("path"), path);

    QString host = profile.host.trimmed();
    if (host.isEmpty()) {
        host = profile.effectiveServerName();
    }
    if (!host.isEmpty()) {
        wsSettings.insert(QStringLiteral("headers"),
                          QJsonObject{{QStringLiteral("Host"), host}});
    }
    return wsSettings;
}

QJsonObject XrayStreamSettings::buildGrpcSettings(const Profile& profile)
{
    QJsonObject grpcSettings;
    QString serviceName = profile.serviceName.trimmed();
    if (serviceName.isEmpty()) {
        serviceName = profile.path.trimmed();
    }
    if (!serviceName.isEmpty()) {
        grpcSettings.insert(QStringLiteral("serviceName"), serviceName);
    }
    return grpcSettings;
}

void XrayStreamSettings::applyTransportLayer(const Profile& profile, const QString& network,
                                             QJsonObject& streamSettings, QString* errorMessage)
{
    streamSettings.insert(QStringLiteral("network"), network);

    if (network == QStringLiteral("ws")) {
        streamSettings.insert(QStringLiteral("wsSettings"), buildWsSettings(profile));
        return;
    }
    if (network == QStringLiteral("grpc")) {
        const QJsonObject grpc = buildGrpcSettings(profile);
        if (grpc.isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("gRPC requires service name.");
            }
            return;
        }
        streamSettings.insert(QStringLiteral("grpcSettings"), grpc);
        return;
    }
    if (network != QStringLiteral("tcp")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported network type: %1").arg(network);
        }
    }
}

void XrayStreamSettings::applySecurityLayer(const Profile& profile, const QString& network,
                                            QJsonObject& streamSettings, QString* errorMessage)
{
    const QString security = normalizedSecurity(profile);

    if (profile.isSecurityReality()) {
        if (network == QStringLiteral("ws")) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("REALITY cannot be used with WebSocket transport.");
            }
            return;
        }
        if (network != QStringLiteral("tcp") && network != QStringLiteral("grpc")) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("REALITY requires network tcp or grpc.");
            }
            return;
        }
        if (profile.publicKey.trimmed().isEmpty() || profile.effectiveServerName().isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("REALITY requires public key and server name.");
            }
            return;
        }
        streamSettings.insert(QStringLiteral("security"), QStringLiteral("reality"));
        streamSettings.insert(QStringLiteral("realitySettings"), buildRealitySettings(profile));
        return;
    }

    if (profile.isSecurityTls() || security == QStringLiteral("tls")) {
        streamSettings.insert(QStringLiteral("security"), QStringLiteral("tls"));
        streamSettings.insert(QStringLiteral("tlsSettings"), buildTlsSettings(profile));
        return;
    }

    if (!security.isEmpty() && security != QStringLiteral("none")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported security type: %1").arg(profile.security);
        }
    }
}

QJsonObject XrayStreamSettings::generate(const Profile& profile, QString* errorMessage)
{
    const QString network = normalizedNetwork(profile);
    QJsonObject streamSettings;
    applyTransportLayer(profile, network, streamSettings, errorMessage);
    if (errorMessage && !errorMessage->isEmpty()) {
        return {};
    }
    applySecurityLayer(profile, network, streamSettings, errorMessage);
    if (errorMessage && !errorMessage->isEmpty()) {
        return {};
    }
    return streamSettings;
}

} // namespace zarya
