#include "geodata/GeoDataVerifier.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

#include <cstdio>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QByteArray checksum =
        "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789  geoip.dat\n"
        "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210  geosite.dat\n";

    const QString parsed =
        zarya::GeoDataVerifier::parseSha256Sum(checksum, QStringLiteral("geoip.dat"));
    if (parsed
        != QStringLiteral("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789")) {
        std::fprintf(stderr, "parseSha256Sum failed for geoip.dat\n");
        return 1;
    }

    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath(QStringLiteral("geoip.dat"));
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        std::fprintf(stderr, "cannot create temp file\n");
        return 1;
    }
    file.write("test");
    file.close();

    const QString hash = zarya::GeoDataVerifier::sha256File(filePath);
    if (hash.isEmpty()) {
        std::fprintf(stderr, "sha256File returned empty\n");
        return 1;
    }

    std::printf("geodata_verifier_test OK\n");
    return 0;
}
