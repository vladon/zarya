#include "domain/Profile.h"
#include "domain/ProfileSourceType.h"
#include "domain/Subscription.h"
#include "diagnostics/DiagnosticsRedactor.h"
#include "subscription/ShareLinkParser.h"
#include "subscription/SubscriptionManager.h"
#include "subscription/SubscriptionParser.h"
#include "storage/ProfileStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>

#include <cstdio>
#include <functional>

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

const char* kVless1 =
    "vless://11111111-1111-1111-1111-111111111111@example.com:443?type=tcp&security=reality&"
    "pbk=yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0&fp=chrome&sni=example.com&sid=a1b2c3d4&"
    "spx=%2F&flow=xtls-rprx-vision#Reality%20Test%201";
const char* kVless2 =
    "vless://22222222-2222-2222-2222-222222222222@example.org:443?type=tcp&security=reality&"
    "pbk=yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0&fp=chrome&sni=example.org&sid=a1b2c3d4&"
    "spx=%2F&flow=xtls-rprx-vision#Reality%20Test%202";

struct ImportCase {
    const char* name;
    QString link;
    zarya::ProtocolType protocol;
    std::function<bool(const zarya::Profile&)> verify;
};

bool containsAnySecret(const QString& text, const QStringList& secrets)
{
    for (const QString& secret : secrets) {
        if (!secret.isEmpty() && text.contains(secret)) {
            return true;
        }
    }
    return false;
}

QString writeTempSubscriptionFile(const QByteArray& body, const QString& marker = {})
{
    const QString suffix = marker.isEmpty() ? QStringLiteral("XXXXXX")
                                             : marker + QStringLiteral("-XXXXXX");
    QTemporaryFile file(QDir::tempPath() + QStringLiteral("/zarya-subscription-") + suffix);
    file.setAutoRemove(false);
    if (!file.open() || file.write(body) != body.size()) {
        return {};
    }
    const QString path = file.fileName();
    file.close();
    return path;
}

bool runValidImportCases()
{
    const QVector<ImportCase> cases{
        {"VLESS",
         QString::fromLatin1(kVless1),
         zarya::ProtocolType::Vless,
         [](const zarya::Profile& profile) {
             return profile.uuidPassword
                        == QStringLiteral("11111111-1111-1111-1111-111111111111")
                    && profile.security == QStringLiteral("reality");
         }},
        {"VMess",
         QStringLiteral(
             "vmess://eyJhZGQiOiJ2bWVzcy5leGFtcGxlLmNvbSIsImFpZCI6IjAiLCJob3N0IjoiIiwiaWQiOi"
             "IxMTExMTExMS0xMTExLTExMTEtMTExMS0xMTExMTExMTExMTEiLCJuZXQiOiJ0Y3AiLCJwYXRo"
             "IjoiIiwicG9ydCI6NDQzLCJwcyI6IlZNZXNzIFRlc3QiLCJzY3kiOiJhdXRvIiwic25pIjoiIiwidG"
             "xzIjoidGxzIiwidHlwZSI6Im5vbmUiLCJ2IjoiMiJ9"),
         zarya::ProtocolType::Vmess,
         [](const zarya::Profile& profile) {
             return profile.securityCipher == QStringLiteral("auto")
                    && profile.address == QStringLiteral("vmess.example.com");
         }},
        {"Trojan",
         QStringLiteral(
             "trojan://trojan-secret@example.com:443?security=tls&sni=example.com#Trojan"),
         zarya::ProtocolType::Trojan,
         [](const zarya::Profile& profile) {
             return profile.password == QStringLiteral("trojan-secret")
                    && profile.security == QStringLiteral("tls");
         }},
        {"Shadowsocks",
         QStringLiteral(
             "ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTpzc2hfc2VjcmV0@ss.example.com:35255?type=tcp#SS"),
         zarya::ProtocolType::Shadowsocks,
         [](const zarya::Profile& profile) {
             return profile.method == QStringLiteral("chacha20-ietf-poly1305")
                    && profile.password == QStringLiteral("ssh_secret");
         }},
        {"Hysteria2",
         QStringLiteral(
             "hysteria2://hy2-secret@hy2.example.com:5225?alpn=h3&fp=firefox&security=tls"
             "&sni=hy2.example.com#Hysteria2"),
         zarya::ProtocolType::Hysteria2,
         [](const zarya::Profile& profile) {
             return profile.password == QStringLiteral("hy2-secret")
                    && profile.alpn == QStringLiteral("h3");
         }},
        {"WireGuard",
         QStringLiteral(
             "wireguard://wg-private-key@wg.example.com:4344?address=10.0.0.2%2F32&mtu=1420"
             "&publickey=wg-peer-public-key#WireGuard"),
         zarya::ProtocolType::WireGuard,
         [](const zarya::Profile& profile) {
             return profile.password == QStringLiteral("wg-private-key")
                    && profile.publicKey == QStringLiteral("wg-peer-public-key")
                    && profile.localAddress == QStringLiteral("10.0.0.2/32");
         }},
    };

    bool ok = true;
    for (const ImportCase& test : cases) {
        const zarya::ShareLinkParseResult parsed = zarya::ShareLinkParser::parse(test.link);
        if (!parsed.ok || parsed.profile.protocol != test.protocol || !test.verify(parsed.profile)) {
            ok &= fail(test.name);
            continue;
        }

        const QString diagnostic = QString::fromUtf8(
            QJsonDocument(zarya::DiagnosticsRedactor::redactProfileSummary(
                              parsed.profile, zarya::DiagnosticsRedactionMode::Strict))
                .toJson(QJsonDocument::Compact));
        const QStringList secrets{
            parsed.profile.address,
            parsed.profile.uuidPassword,
            parsed.profile.password,
            parsed.profile.publicKey,
            parsed.profile.preSharedKey,
        };
        if (containsAnySecret(diagnostic, secrets)) {
            ok &= fail(test.name);
        } else {
            ok &= pass(test.name);
        }
    }
    return ok;
}

bool runInvalidAndRedactionCases()
{
    const QVector<ImportCase> cases{
        {"Invalid VLESS",
         QStringLiteral("vless://@example.com:443?token=vless-secret"),
         zarya::ProtocolType::Vless,
         {}},
        {"Invalid VMess",
         QStringLiteral("vmess://not-valid-base64-vmess-secret!"),
         zarya::ProtocolType::Vmess,
         {}},
        {"Invalid Trojan",
         QStringLiteral("trojan://@example.com:443?token=trojan-secret"),
         zarya::ProtocolType::Trojan,
         {}},
        {"Invalid Shadowsocks",
         QStringLiteral("ss://invalid-ss-secret@example.com:not-a-port"),
         zarya::ProtocolType::Shadowsocks,
         {}},
        {"Invalid Hysteria2",
         QStringLiteral("hysteria2://@example.com:443?auth="),
         zarya::ProtocolType::Hysteria2,
         {}},
        {"Invalid WireGuard",
         QStringLiteral("wireguard://wg-private-secret@example.com:51820"),
         zarya::ProtocolType::WireGuard,
         {}},
    };
    const QStringList secrets{
        QStringLiteral("vless-secret"),
        QStringLiteral("vmess-secret"),
        QStringLiteral("trojan-secret"),
        QStringLiteral("ss-secret"),
        QStringLiteral("wg-private-secret"),
        QStringLiteral("://"),
    };

    bool ok = true;
    for (const ImportCase& test : cases) {
        const zarya::ShareLinkParseResult parsed = zarya::ShareLinkParser::parse(test.link);
        if (parsed.ok || parsed.error.isEmpty() || containsAnySecret(parsed.error, secrets)) {
            ok &= fail(test.name);
        } else {
            ok &= pass(test.name);
        }
    }
    return ok;
}

bool runEdgeCases()
{
    bool ok = true;

    const zarya::ShareLinkParseResult ipv6 = zarya::ShareLinkParser::parse(QStringLiteral(
        "vless://11111111-1111-1111-1111-111111111111@[2001:db8::1]:8443?security=tls"));
    if (!ipv6.ok || ipv6.profile.address != QStringLiteral("2001:db8::1")
        || ipv6.profile.port != 8443) {
        ok &= fail("Bracketed IPv6 host");
    } else {
        ok &= pass("Bracketed IPv6 host");
    }

    const zarya::ShareLinkParseResult duplicate = zarya::ShareLinkParser::parse(QStringLiteral(
        "trojan://secret@example.com:443?sni=first.example&sni=second.example"));
    if (!duplicate.ok || duplicate.profile.sni != QStringLiteral("second.example")) {
        ok &= fail("Duplicate query keys have deterministic last-value semantics");
    } else {
        ok &= pass("Duplicate query keys have deterministic last-value semantics");
    }

    const zarya::ShareLinkParseResult unsupported = zarya::ShareLinkParser::parse(QStringLiteral(
        "ss://YWVzLTI1Ni1nY206dGVzdA==@127.0.0.1:8388/?plugin=obfs-local%3Bobfs%3Dhttp"));
    if (!unsupported.ok || unsupported.profile.unsupportedReason.isEmpty()) {
        ok &= fail("Unsupported Shadowsocks plugin is preserved but marked");
    } else {
        ok &= pass("Unsupported Shadowsocks plugin is preserved but marked");
    }

    return ok;
}

bool runSubscriptionParserCases()
{
    bool ok = true;
    const QByteArray plainBody = QByteArray(kVless1) + "\n" + kVless2;
    const zarya::SubscriptionParseResult plain = zarya::SubscriptionParser::parse(plainBody);
    const zarya::SubscriptionParseResult encoded =
        zarya::SubscriptionParser::parse(plainBody.toBase64());
    if (!plain.success || plain.profiles.size() != 2 || !encoded.success
        || encoded.profiles.size() != 2) {
        ok &= fail("Plain and base64 subscriptions");
    } else {
        ok &= pass("Plain and base64 subscriptions");
    }

    const QByteArray rawSecret =
        "https://user:subscription-secret@example.invalid/path?token=raw-link-secret";
    const QByteArray mixed = QByteArray(kVless1) + "\n" + rawSecret;
    const zarya::SubscriptionParseResult mixedResult = zarya::SubscriptionParser::parse(mixed);
    const QString diagnostics =
        mixedResult.warnings.join(QLatin1Char('\n')) + mixedResult.errorMessage;
    if (!mixedResult.success || mixedResult.profiles.size() != 1 || mixedResult.skippedLines != 1
        || containsAnySecret(diagnostics,
                            {QString::fromUtf8(rawSecret),
                             QStringLiteral("subscription-secret"),
                             QStringLiteral("raw-link-secret")})) {
        ok &= fail("Mixed-validity subscription has redacted diagnostics");
    } else {
        ok &= pass("Mixed-validity subscription has redacted diagnostics");
    }

    const zarya::SubscriptionParseResult malformed =
        zarya::SubscriptionParser::parse(QByteArray("%%% malformed base64 %%%"));
    if (malformed.success || malformed.errorMessage.isEmpty()) {
        ok &= fail("Malformed subscription encoding");
    } else {
        ok &= pass("Malformed subscription encoding");
    }
    return ok;
}

bool runSubscriptionUpdateCases()
{
    bool ok = true;
    const QByteArray plainBody = QByteArray(kVless1) + "\n" + kVless2;
    const QString goodPath = writeTempSubscriptionFile(plainBody, QStringLiteral("url-secret"));
    const QString badPath =
        writeTempSubscriptionFile(QByteArray("not a subscription"), QStringLiteral("url-secret"));
    if (goodPath.isEmpty() || badPath.isEmpty()) {
        return fail("Could not create subscription fixtures");
    }

    zarya::Subscription subscription = zarya::Subscription::createDefault();
    subscription.name = QStringLiteral("Test subscription");
    subscription.url = QUrl::fromLocalFile(goodPath).toString();
    subscription.enabled = true;

    QVector<zarya::Profile> profiles;
    zarya::SubscriptionManager manager;
    QStringList logs;
    QObject::connect(&manager, &zarya::SubscriptionManager::logLine,
                     [&logs](const QString& line) { logs.append(line); });

    const zarya::SubscriptionUpdateResult first =
        manager.updateSubscription(subscription, profiles);
    if (!first.success || first.stats.addedProfiles != 2) {
        ok &= fail("Initial subscription update");
    } else {
        ok &= pass("Initial subscription update");
    }

    const zarya::SubscriptionUpdateResult second =
        manager.updateSubscription(subscription, profiles);
    if (!second.success || second.stats.updatedProfiles != 2 || second.stats.addedProfiles != 0) {
        ok &= fail("Repeated subscription update");
    } else {
        ok &= pass("Repeated subscription update");
    }

    const QString onePath =
        writeTempSubscriptionFile(QByteArray(kVless1), QStringLiteral("url-secret"));
    subscription.url = QUrl::fromLocalFile(onePath).toString();
    const zarya::SubscriptionUpdateResult reduced =
        manager.updateSubscription(subscription, profiles);
    if (!reduced.success || reduced.stats.updatedProfiles != 1
        || reduced.stats.markedMissingProfiles != 1) {
        ok &= fail("Removed subscription profile is marked missing");
    } else {
        ok &= pass("Removed subscription profile is marked missing");
    }

    zarya::Profile manual = zarya::Profile::createDefault();
    manual.name = QStringLiteral("Manual profile");
    profiles.append(manual);
    subscription.url = QUrl::fromLocalFile(goodPath).toString();
    const zarya::SubscriptionUpdateResult withManual =
        manager.updateSubscription(subscription, profiles);
    bool manualKept = false;
    for (const zarya::Profile& profile : profiles) {
        manualKept = manualKept
                     || (profile.isManual() && profile.name == QStringLiteral("Manual profile"));
    }
    if (!withManual.success || !manualKept) {
        ok &= fail("Manual profiles survive subscription refresh");
    } else {
        ok &= pass("Manual profiles survive subscription refresh");
    }

    const QVector<zarya::Profile> profilesBeforeFailure = profiles;
    subscription.url = QUrl::fromLocalFile(badPath).toString();
    const zarya::SubscriptionUpdateResult failed =
        manager.updateSubscription(subscription, profiles);
    bool unchanged = profiles.size() == profilesBeforeFailure.size();
    for (int i = 0; unchanged && i < profiles.size(); ++i) {
        unchanged = profiles.at(i).id == profilesBeforeFailure.at(i).id
                    && profiles.at(i).sourceKey == profilesBeforeFailure.at(i).sourceKey
                    && profiles.at(i).deletedBySubscriptionUpdate
                           == profilesBeforeFailure.at(i).deletedBySubscriptionUpdate;
    }
    if (failed.success || !unchanged) {
        ok &= fail("Failed refresh preserves existing profiles");
    } else {
        ok &= pass("Failed refresh preserves existing profiles");
    }

    if (containsAnySecret(logs.join(QLatin1Char('\n')),
                          {goodPath, badPath, QStringLiteral("url-secret")})) {
        ok &= fail("Subscription logs redact source URLs");
    } else {
        ok &= pass("Subscription logs redact source URLs");
    }

    QFile::remove(goodPath);
    QFile::remove(badPath);
    QFile::remove(onePath);
    return ok;
}

bool runPersistedProfileCompatibilityCase()
{
    QTemporaryDir directory;
    if (!directory.isValid()) {
        return fail("Could not create profile store fixture directory");
    }
    const QString path = directory.filePath(QStringLiteral("profiles.json"));

    zarya::Profile original =
        zarya::ShareLinkParser::parse(QStringLiteral(
            "wireguard://persisted-private@wg.example.com:51820?address=10.0.0.2%2F32"
            "&publickey=persisted-public&allowedips=0.0.0.0%2F0&mtu=1380"))
            .profile;
    zarya::ProfileStore store(path);
    QString error;
    if (!store.save({original}, &error)) {
        return fail("Could not save imported profile");
    }
    const QVector<zarya::Profile> loaded = store.load(&error);
    if (loaded.size() != 1 || loaded.constFirst().protocol != zarya::ProtocolType::WireGuard
        || loaded.constFirst().password != original.password
        || loaded.constFirst().publicKey != original.publicKey
        || loaded.constFirst().localAddress != original.localAddress
        || loaded.constFirst().allowedIps != original.allowedIps
        || loaded.constFirst().mtu != original.mtu) {
        return fail("Imported profile persistence round-trip");
    }
    return pass("Imported profile persistence round-trip");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    bool ok = true;
    ok &= runValidImportCases();
    ok &= runInvalidAndRedactionCases();
    ok &= runEdgeCases();
    ok &= runSubscriptionParserCases();
    ok &= runSubscriptionUpdateCases();
    ok &= runPersistedProfileCompatibilityCase();
    return ok ? 0 : 1;
}
