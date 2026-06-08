#include "cores/CoreChecksum.h"

#include <QCryptographicHash>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

namespace zarya {

std::optional<QString> CoreChecksum::findChecksumAssetName(const QVector<CoreAsset>& assets,
                                                           const QString& archiveName)
{
    const QString lowerArchive = archiveName.toLower();
    QStringList candidates = {
        archiveName + QStringLiteral(".sha256"),
        archiveName + QStringLiteral(".sha256sum"),
        QStringLiteral("SHA256SUMS"),
        QStringLiteral("sha256sums"),
        QStringLiteral("checksums.txt"),
        QStringLiteral("sha256sum.txt"),
    };

    for (const QString& candidate : candidates) {
        for (const CoreAsset& asset : assets) {
            if (asset.name.compare(candidate, Qt::CaseInsensitive) == 0) {
                return asset.name;
            }
        }
    }

    for (const CoreAsset& asset : assets) {
        const QString lowerName = asset.name.toLower();
        if (lowerName.contains(QStringLiteral("sha256")) && lowerName.contains(lowerArchive)) {
            return asset.name;
        }
    }

    return std::nullopt;
}

std::optional<QString> CoreChecksum::parseExpectedSha256(const QByteArray& checksumBody,
                                                         const QString& archiveName)
{
    const QString text = QString::fromUtf8(checksumBody);
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                         Qt::SkipEmptyParts);
    static const QRegularExpression hexPattern(QStringLiteral(R"(([0-9a-fA-F]{64}))"));

    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.contains(archiveName, Qt::CaseInsensitive)) {
            const QRegularExpressionMatch match = hexPattern.match(trimmed);
            if (match.hasMatch()) {
                return match.captured(1).toLower();
            }
        }
        static const QRegularExpression parenPattern(
            QStringLiteral(R"(\(([^)]+)\)\s*=\s*([0-9a-fA-F]{64}))"));
        const QRegularExpressionMatch parenMatch = parenPattern.match(trimmed);
        if (parenMatch.hasMatch()
            && parenMatch.captured(1).compare(archiveName, Qt::CaseInsensitive) == 0) {
            return parenMatch.captured(2).toLower();
        }
    }

    if (lines.size() == 1) {
        const QRegularExpressionMatch match = hexPattern.match(lines.first());
        if (match.hasMatch()) {
            return match.captured(1).toLower();
        }
    }

    return std::nullopt;
}

QString CoreChecksum::sha256OfFile(const QString& filePath, QString* errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool CoreChecksum::verifyFileSha256(const QString& filePath, const QString& expectedSha256,
                                    QString* errorMessage)
{
    const QString actual = sha256OfFile(filePath, errorMessage);
    if (actual.isEmpty()) {
        return false;
    }
    if (actual.compare(expectedSha256.trimmed(), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Checksum mismatch for %1.").arg(filePath);
        }
        return false;
    }
    return true;
}

} // namespace zarya
