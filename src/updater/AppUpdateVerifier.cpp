#include "updater/AppUpdateVerifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

namespace zarya {

QString AppUpdateVerifier::sha256OfFile(const QString& filePath, QString* errorMessage)
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

bool AppUpdateVerifier::verifySha256(const QString& filePath, const QString& expectedHash,
                                     QString* errorMessage)
{
    const QString actual = sha256OfFile(filePath, errorMessage);
    if (actual.isEmpty()) {
        return false;
    }
    if (actual.compare(expectedHash.trimmed(), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("SHA256 checksum mismatch for %1.").arg(QFileInfo(filePath).fileName());
        }
        return false;
    }
    return true;
}

QStringList AppUpdateVerifier::signatureWarnings(const AppUpdateAsset& asset)
{
    QStringList warnings;
    if (asset.signature.type.isEmpty() || asset.signature.type == QStringLiteral("none")) {
        return warnings;
    }
    warnings.append(QStringLiteral(
        "Signature verification is not implemented for signature type \"%1\".")
                        .arg(asset.signature.type));
    return warnings;
}

bool AppUpdateVerifier::canDownloadAsset(const AppUpdateAsset& asset, bool allowUnsigned)
{
    if (!asset.sha256.isEmpty()) {
        return true;
    }
    return allowUnsigned;
}

} // namespace zarya
