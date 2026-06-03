#include "core/XrayConfigTestHelpers.h"
#include "core/XrayVlessGenerator.h"
#include "storage/ProfileStore.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdio>

namespace {

bool fail(const char* message)
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    return false;
}

bool pass(const char* message)
{
    std::fprintf(stdout, "PASS: %s\n", message);
    return true;
}

QJsonObject loadJsonObject(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return {};
    }
    return document.object();
}

QJsonObject firstProxyOutbound(const QJsonObject& config)
{
    const QJsonArray outbounds = config.value(QStringLiteral("outbounds")).toArray();
    if (outbounds.isEmpty()) {
        return {};
    }
    return outbounds.first().toObject();
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QString examplesDir = QStringLiteral(ZARYA_EXAMPLES_DIR);
    bool ok = true;

    const zarya::Profile sample = zarya::testhelpers::sampleVlessRealityProfile();
    QString generateError;
    const zarya::ConfigGenerationResult generated = zarya::XrayVlessGenerator::generate(sample);
    if (!generated.success) {
        ok &= fail(generated.errorMessage.toUtf8().constData());
    } else {
        ok &= pass("Sample REALITY profile generates config");
    }

    const QJsonObject proxyOutbound = firstProxyOutbound(generated.config);
    if (!zarya::testhelpers::proxyOutboundHasReality(proxyOutbound)) {
        ok &= fail("Proxy outbound missing REALITY stream settings");
    } else {
        ok &= pass("Proxy outbound contains REALITY");
    }

    const QJsonObject reality =
        proxyOutbound.value(QStringLiteral("streamSettings"))
            .toObject()
            .value(QStringLiteral("realitySettings"))
            .toObject();
    QString missingKey;
    const QStringList realityKeys = {QStringLiteral("publicKey"), QStringLiteral("serverName"),
                                     QStringLiteral("shortId"), QStringLiteral("fingerprint"),
                                     QStringLiteral("spiderX")};
    if (!zarya::testhelpers::jsonContainsKeys(reality, realityKeys, &missingKey)) {
        ok &= fail(QStringLiteral("Missing reality key: %1").arg(missingKey).toUtf8().constData());
    } else {
        ok &= pass("REALITY settings include required client keys");
    }

    const QJsonObject users =
        proxyOutbound.value(QStringLiteral("settings"))
            .toObject()
            .value(QStringLiteral("vnext"))
            .toArray()
            .first()
            .toObject()
            .value(QStringLiteral("users"))
            .toArray()
            .first()
            .toObject();
    if (users.value(QStringLiteral("flow")).toString()
        != QStringLiteral("xtls-rprx-vision")) {
        ok &= fail("VLESS user flow is not xtls-rprx-vision");
    } else {
        ok &= pass("VLESS user flow is xtls-rprx-vision");
    }

    QString fileError;
    const QJsonObject expectedConfig =
        loadJsonObject(examplesDir + QStringLiteral("/xray-vless-reality.sample.json"), &fileError);
    if (expectedConfig.isEmpty()) {
        ok &= fail(fileError.toUtf8().constData());
    } else {
        const QJsonObject expectedProxy = firstProxyOutbound(expectedConfig);
        const QJsonObject expectedReality =
            expectedProxy.value(QStringLiteral("streamSettings"))
                .toObject()
                .value(QStringLiteral("realitySettings"))
                .toObject();
        if (reality.value(QStringLiteral("publicKey")) != expectedReality.value(QStringLiteral("publicKey"))
            || reality.value(QStringLiteral("serverName"))
                   != expectedReality.value(QStringLiteral("serverName"))) {
            ok &= fail("Generated REALITY settings differ from example file");
        } else {
            ok &= pass("Generated REALITY settings match example publicKey and serverName");
        }
    }

    zarya::ProfileStore store(examplesDir + QStringLiteral("/profiles-vless-reality.sample.json"));
    const QVector<zarya::Profile> loaded = store.load(&fileError);
    if (loaded.isEmpty()) {
        ok &= fail("Failed to load sample profiles file");
    } else {
        ok &= pass("Backward-compatible profile JSON loads");
        const auto loadedResult = zarya::XrayVlessGenerator::generate(loaded.first());
        if (!loadedResult.success) {
            ok &= fail("Loaded sample profile failed Xray generation");
        } else {
            ok &= pass("Loaded sample profile generates Xray config");
        }
    }

    return ok ? 0 : 1;
}
