#include "cores/CoreChecksum.h"

#include <QCoreApplication>

#include <cstdio>

namespace {

int fail(const char* message)
{
    std::fprintf(stderr, "%s\n", message);
    return 1;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QString archive = QStringLiteral("Xray-macos-arm64-v8a.zip");
    QVector<zarya::CoreAsset> assets;
    assets.append({QStringLiteral("Xray-macos-arm64-v8a.zip"), {}, 0, {}, {}});
    assets.append({QStringLiteral("Xray-macos-arm64-v8a.zip.dgst"), {}, 0, {}, {}});
    assets.append({QStringLiteral("Xray-windows-64.zip"), {}, 0, {}, {}});
    assets.append({QStringLiteral("Xray-windows-64.zip.dgst"), {}, 0, {}, {}});

    const auto dgstName = zarya::CoreChecksum::findChecksumAssetName(assets, archive);
    if (!dgstName.has_value() || *dgstName != QStringLiteral("Xray-macos-arm64-v8a.zip.dgst")) {
        return fail("findChecksumAssetName failed for Xray .dgst sidecar");
    }

    const QByteArray dgstBody =
        "MD5= c7253cf3e605d261f5e1a4a55f447d9d\n"
        "SHA1= f02425f9dc1e353388dc9042914b7a0a809b0272\n"
        "SHA2-256= 2e93a67e8aa1936ecefb307e120830fcbd4c643ab9b1c46a2d0838d5f8409eaf\n"
        "SHA2-512= 55683e386cbc5028001a65ee666660ff2bd8867ddf46edb8fe9aa9c1e22790c3"
        "a587dbb4b989bccb2248e29e129bc39d7675cc745407764d17a42df7c844040d\n";

    const auto parsed = zarya::CoreChecksum::parseExpectedSha256(dgstBody, archive);
    if (!parsed.has_value()
        || *parsed
            != QStringLiteral("2e93a67e8aa1936ecefb307e120830fcbd4c643ab9b1c46a2d0838d5f8409eaf")) {
        return fail("parseExpectedSha256 failed for openssl SHA2-256= dgst format");
    }

    QVector<zarya::CoreAsset> shaAssets;
    shaAssets.append({QStringLiteral("foo.zip"), {}, 0, {}, {}});
    shaAssets.append({QStringLiteral("foo.zip.sha256"), {}, 0, {}, {}});
    const auto shaName =
        zarya::CoreChecksum::findChecksumAssetName(shaAssets, QStringLiteral("foo.zip"));
    if (!shaName.has_value() || *shaName != QStringLiteral("foo.zip.sha256")) {
        return fail("findChecksumAssetName failed for .sha256 sidecar");
    }

    const QByteArray sumsBody =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  other.zip\n"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb  foo.zip\n";
    const auto sumsParsed =
        zarya::CoreChecksum::parseExpectedSha256(sumsBody, QStringLiteral("foo.zip"));
    if (!sumsParsed.has_value()
        || *sumsParsed
            != QStringLiteral("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")) {
        return fail("parseExpectedSha256 failed for SHA256SUMS-style line");
    }

    const auto digest = zarya::CoreChecksum::parseGitHubDigest(QStringLiteral(
        "sha256:73e8967b0fc08e17bce4263ca56ebc394822401a16497a1c4e02316c888202ab"));
    if (!digest.has_value()
        || *digest
            != QStringLiteral("73e8967b0fc08e17bce4263ca56ebc394822401a16497a1c4e02316c888202ab")) {
        return fail("parseGitHubDigest failed for sha256 digest");
    }
    if (zarya::CoreChecksum::parseGitHubDigest(QStringLiteral("md5:deadbeef")).has_value()) {
        return fail("parseGitHubDigest should reject non-sha256 digests");
    }

    std::printf("core_checksum_test OK\n");
    return 0;
}
