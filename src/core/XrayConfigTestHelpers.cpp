#include "core/XrayConfigTestHelpers.h"

#include "core/XrayVlessGenerator.h"

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

QJsonObject sampleVlessRealityProxyOutbound()
{
    QString error;
    return XrayVlessGenerator::buildProxyOutbound(sampleVlessRealityProfile(), &error);
}

QString generateXrayConfigJson(const Profile& profile, QString* errorMessage)
{
    const ConfigGenerationResult result = XrayVlessGenerator::generate(profile);
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
