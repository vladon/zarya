#include "core/XrayConfigTestHelpers.h"

#include "core/XrayAdapter.h"

#include <QJsonDocument>

namespace zarya::testhelpers {

Profile sampleVlessRealityProfile()
{
    Profile profile;
    profile.id = QStringLiteral("sample-reality");
    profile.name = QStringLiteral("VLESS REALITY sample");
    profile.protocol = ProtocolType::Vless;
    profile.coreType = CoreType::Xray;
    profile.address = QStringLiteral("proxy.example.com");
    profile.port = 443;
    profile.uuidPassword = QStringLiteral("11111111-1111-1111-1111-111111111111");
    profile.security = QStringLiteral("reality");
    profile.network = QStringLiteral("tcp");
    profile.flow = QStringLiteral("xtls-rprx-vision");
    profile.serverName = QStringLiteral("www.microsoft.com");
    profile.publicKey =
        QStringLiteral("yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0");
    profile.shortId = QStringLiteral("a1b2c3d4");
    profile.fingerprint = QStringLiteral("chrome");
    profile.spiderX = QStringLiteral("/");
    profile.enabled = true;
    return profile;
}

Profile sampleVmessTcpTlsProfile()
{
    Profile profile;
    profile.protocol = ProtocolType::Vmess;
    profile.coreType = CoreType::Xray;
    profile.name = QStringLiteral("VMess TCP TLS");
    profile.address = QStringLiteral("vmess.example.com");
    profile.port = 443;
    profile.uuidPassword = QStringLiteral("11111111-1111-1111-1111-111111111111");
    profile.security = QStringLiteral("tls");
    profile.serverName = QStringLiteral("vmess.example.com");
    profile.network = QStringLiteral("tcp");
    profile.securityCipher = QStringLiteral("auto");
    profile.enabled = true;
    return profile;
}

Profile sampleTrojanTlsProfile()
{
    Profile profile;
    profile.protocol = ProtocolType::Trojan;
    profile.coreType = CoreType::Xray;
    profile.name = QStringLiteral("Trojan TLS");
    profile.address = QStringLiteral("trojan.example.com");
    profile.port = 443;
    profile.password = QStringLiteral("secret");
    profile.security = QStringLiteral("tls");
    profile.serverName = QStringLiteral("trojan.example.com");
    profile.network = QStringLiteral("tcp");
    profile.enabled = true;
    return profile;
}

Profile sampleShadowsocksProfile()
{
    Profile profile;
    profile.protocol = ProtocolType::Shadowsocks;
    profile.coreType = CoreType::Xray;
    profile.name = QStringLiteral("Shadowsocks");
    profile.address = QStringLiteral("ss.example.com");
    profile.port = 8388;
    profile.method = QStringLiteral("2022-blake3-aes-128-gcm");
    profile.password = QStringLiteral("password");
    profile.enabled = true;
    return profile;
}

QJsonObject sampleVlessRealityProxyOutbound()
{
    QString error;
    XrayAdapter adapter;
    return adapter.generateOutbound(sampleVlessRealityProfile(), &error);
}

QString generateXrayConfigJson(const Profile& profile, QString* errorMessage)
{
    XrayAdapter adapter;
    const ConfigGenerationResult result = adapter.generateConfig(profile);
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = result.errorMessage;
        }
        return {};
    }
    return QString::fromUtf8(QJsonDocument(result.config).toJson(QJsonDocument::Indented));
}

bool proxyOutboundHasReality(const QJsonObject& proxyOutbound)
{
    const QJsonObject stream =
        proxyOutbound.value(QStringLiteral("streamSettings")).toObject();
    if (stream.value(QStringLiteral("security")).toString()
        != QStringLiteral("reality")) {
        return false;
    }
    const QJsonObject reality = stream.value(QStringLiteral("realitySettings")).toObject();
    return !reality.isEmpty() && reality.contains(QStringLiteral("publicKey"))
           && reality.contains(QStringLiteral("serverName"));
}

bool jsonContainsKeys(const QJsonObject& object, const QStringList& keys, QString* missingKey)
{
    for (const QString& key : keys) {
        if (!object.contains(key)) {
            if (missingKey) {
                *missingKey = key;
            }
            return false;
        }
    }
    return true;
}

} // namespace zarya::testhelpers
