#include "geodata/GeoDataVerifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace zarya {

bool GeoDataVerifier::isValidSha256Hex(const QString& value)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-fA-F]{64}$"));
    return pattern.match(value.trimmed()).hasMatch();
}

QString GeoDataVerifier::sha256File(const QString& path, QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to read file for SHA256: %1").arg(path);
        }
        return {};
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString GeoDataVerifier::parseSha256Sum(const QByteArray& content, const QString& expectedFileName)
{
    const QStringList lines = QString::fromUtf8(content).split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                             Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            continue;
        }
        const QString hash = parts.first();
        const QString fileName = parts.last();
        if (!isValidSha256Hex(hash)) {
            continue;
        }
        if (fileName == expectedFileName || fileName.endsWith(QLatin1Char('/') + expectedFileName)) {
            return hash.toLower();
        }
    }
    return {};
}

bool GeoDataVerifier::verifySha256(const QString& filePath, const QString& expectedSha256,
                                   QString* errorMessage)
{
    if (!isValidSha256Hex(expectedSha256)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid expected SHA256 checksum.");
        }
        return false;
    }

    QString hashError;
    const QString actual = sha256File(filePath, &hashError);
    if (actual.isEmpty()) {
        if (errorMessage) {
            *errorMessage = hashError;
        }
        return false;
    }

    if (actual.compare(expectedSha256.trimmed(), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SHA256 mismatch.");
        }
        return false;
    }
    return true;
}

} // namespace zarya
