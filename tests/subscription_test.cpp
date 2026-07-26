#include "domain/Profile.h"
#include "domain/ProfileSourceType.h"
#include "domain/Subscription.h"
#include "subscription/ShareLinkParser.h"
#include "subscription/SubscriptionManager.h"
#include "subscription/SubscriptionParser.h"

#include <QCoreApplication>
#include <QTemporaryFile>
#include <QUrl>

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

const char* kVless1 =
    "vless://11111111-1111-1111-1111-111111111111@example.com:443?type=tcp&security=reality&"
    "pbk=yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0&fp=chrome&sni=example.com&sid=a1b2c3d4&"
    "spx=%2F&flow=xtls-rprx-vision#Reality%20Test%201";
const char* kVless2 =
    "vless://22222222-2222-2222-2222-222222222222@example.org:443?type=tcp&security=reality&"
    "pbk=yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0&fp=chrome&sni=example.org&sid=a1b2c3d4&"
    "spx=%2F&flow=xtls-rprx-vision#Reality%20Test%202";

QString writeTempSubscriptionFile(const QByteArray& body)
{
    auto* file = new QTemporaryFile();
    file->setAutoRemove(false);
    if (!file->open()) {
        delete file;
        return {};
    }
    file->write(body);
    file->close();
    const QString path = file->fileName();
    delete file;
    return path;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    bool ok = true;

    const QByteArray plainBody = QByteArray(kVless1) + "\n" + kVless2;
    const zarya::SubscriptionParseResult plain = zarya::SubscriptionParser::parse(plainBody);
    if (!plain.success || plain.profiles.size() != 2) {
        ok &= fail("Plain subscription should parse two profiles");
    } else {
        ok &= pass("Plain subscription parses two vless profiles");
    }

    const QByteArray encoded = plainBody.toBase64();
    const zarya::SubscriptionParseResult b64 = zarya::SubscriptionParser::parse(encoded);
    if (!b64.success || b64.profiles.size() != 2) {
        ok &= fail("Base64 subscription should parse two profiles");
    } else {
        ok &= pass("Base64 subscription parses two vless profiles");
    }

    const zarya::ShareLinkParseResult vmess =
        zarya::ShareLinkParser::parse(QStringLiteral(
            "vmess://eyJhZGQiOiJ2bWVzcy5leGFtcGxlLmNvbSIsImFpZCI6IjAiLCJob3N0IjoiIiwiaWQiOi"
            "IxMTExMTExMS0xMTExLTExMTEtMTExMS0xMTExMTExMTExMTEiLCJuZXQiOiJ0Y3AiLCJwYXRo"
            "IjoiIiwicG9ydCI6NDQzLCJwcyI6IlZNZXNzIFRlc3QiLCJzY3kiOiJhdXRvIiwic25pIjoiIiwidG"
            "xzIjoidGxzIiwidHlwZSI6Im5vbmUiLCJ2IjoiMiJ9"));
    if (!vmess.ok || vmess.profile.protocol != zarya::ProtocolType::Vmess) {
        ok &= fail("vmess:// link should parse");
    } else if (vmess.profile.securityCipher != QStringLiteral("auto")) {
        ok &= fail("vmess:// should map scy to securityCipher");
    } else {
        ok &= pass("vmess:// link parses into profile");
    }

    const zarya::ShareLinkParseResult trojan = zarya::ShareLinkParser::parse(QStringLiteral(
        "trojan://PASSWORD@example.com:443?security=tls&sni=example.com#Trojan%20Test"));
    if (!trojan.ok || trojan.profile.protocol != zarya::ProtocolType::Trojan
        || trojan.profile.password != QStringLiteral("PASSWORD")
        || trojan.profile.security != QStringLiteral("tls")) {
        ok &= fail("trojan:// link should parse with password and tls");
    } else {
        ok &= pass("trojan:// link parses into runnable profile fields");
    }

    const zarya::ShareLinkParseResult trojanReality = zarya::ShareLinkParser::parse(QStringLiteral(
        "trojan://PASSWORD@trojan.example.com:56442?fp=firefox"
        "&pbk=uREwNdwudj2vLUBJrveW3fx6xDABilwJjGqaVV26gQc&security=reality"
        "&sid=2c5dcd8cab237e&sni=example.com&spx=%2Fpath&type=tcp#Trojan%20Reality"));
    if (!trojanReality.ok || trojanReality.profile.protocol != zarya::ProtocolType::Trojan
        || trojanReality.profile.security != QStringLiteral("reality")
        || trojanReality.profile.publicKey
            != QStringLiteral("uREwNdwudj2vLUBJrveW3fx6xDABilwJjGqaVV26gQc")
        || trojanReality.profile.shortId != QStringLiteral("2c5dcd8cab237e")
        || trojanReality.profile.spiderX != QStringLiteral("/path")
        || trojanReality.profile.fingerprint != QStringLiteral("firefox")
        || trojanReality.profile.sni != QStringLiteral("example.com")
        || trojanReality.profile.network != QStringLiteral("tcp")) {
        ok &= fail("trojan:// Reality link should parse pbk/sid/spx/fp/sni");
    } else {
        ok &= pass("trojan:// Reality link parses Reality query params");
    }

    const zarya::ShareLinkParseResult ssPlugin = zarya::ShareLinkParser::parse(QStringLiteral(
        "ss://YWVzLTI1Ni1nY206dGVzdA==@127.0.0.1:8388/?plugin=obfs-local%3Bobfs%3Dhttp"));
    if (!ssPlugin.ok || ssPlugin.profile.unsupportedReason.isEmpty()) {
        ok &= fail("ss:// with plugin should set unsupportedReason");
    } else if (ssPlugin.profile.port != 8388 || ssPlugin.profile.address != QStringLiteral("127.0.0.1")) {
        ok &= fail("ss:// with plugin path should still parse host/port");
    } else {
        ok &= pass("ss:// plugin link marks unsupported reason");
    }

    const zarya::ShareLinkParseResult ssQuery = zarya::ShareLinkParser::parse(QStringLiteral(
        "ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTp0ZXN0LXBhc3N3b3Jk@ss.example.com:35255?type=tcp#"
        "ss-query-test"));
    if (!ssQuery.ok || ssQuery.profile.protocol != zarya::ProtocolType::Shadowsocks
        || ssQuery.profile.port != 35255
        || ssQuery.profile.address != QStringLiteral("ss.example.com")
        || ssQuery.profile.method != QStringLiteral("chacha20-ietf-poly1305")
        || ssQuery.profile.password != QStringLiteral("test-password")
        || ssQuery.profile.network != QStringLiteral("tcp")
        || ssQuery.profile.name != QStringLiteral("ss-query-test")) {
        ok &= fail("ss:// SIP002 with ?type= query should parse port/method/name");
    } else {
        ok &= pass("ss:// SIP002 query port parses correctly");
    }

    const QString tempPath = writeTempSubscriptionFile(plainBody);
    if (tempPath.isEmpty()) {
        ok &= fail("Could not create temp subscription file");
    } else {
        zarya::Subscription subscription = zarya::Subscription::createDefault();
        subscription.name = QStringLiteral("Test sub");
        subscription.url = QUrl::fromLocalFile(tempPath).toString();
        subscription.enabled = true;

        QVector<zarya::Profile> profiles;
        zarya::SubscriptionManager manager;

        const zarya::SubscriptionUpdateResult first =
            manager.updateSubscription(subscription, profiles);
        if (!first.success || first.stats.addedProfiles != 2) {
            ok &= fail("First subscription update should add 2 profiles");
        } else {
            ok &= pass("First update adds 2 profiles");
        }

        const zarya::SubscriptionUpdateResult second =
            manager.updateSubscription(subscription, profiles);
        if (!second.success || second.stats.updatedProfiles != 2 || second.stats.addedProfiles != 0) {
            ok &= fail("Second subscription update should update 2 profiles");
        } else {
            ok &= pass("Second update updates 2 profiles");
        }

        const QByteArray oneLine = QByteArray(kVless1);
        subscription.url = QUrl::fromLocalFile(writeTempSubscriptionFile(oneLine)).toString();
        const zarya::SubscriptionUpdateResult third =
            manager.updateSubscription(subscription, profiles);
        if (!third.success || third.stats.updatedProfiles != 1 || third.stats.markedMissingProfiles != 1) {
            ok &= fail("Third update should mark one profile missing");
        } else {
            ok &= pass("Removed node is marked missing, not hard-deleted");
        }

        profiles.clear();
        zarya::Profile manual = zarya::Profile::createDefault();
        manual.name = QStringLiteral("Manual profile");
        profiles.append(manual);
        subscription.url = QUrl::fromLocalFile(tempPath).toString();
        const zarya::SubscriptionUpdateResult withManual =
            manager.updateSubscription(subscription, profiles);
        bool manualKept = false;
        for (const zarya::Profile& profile : profiles) {
            if (profile.isManual() && profile.name == QStringLiteral("Manual profile")) {
                manualKept = true;
            }
        }
        if (!withManual.success || !manualKept) {
            ok &= fail("Manual profiles should survive subscription update");
        } else {
            ok &= pass("Manual profiles survive subscription update");
        }
    }

    return ok ? 0 : 1;
}
