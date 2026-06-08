#include "backup/BackupValidator.h"

#include "packaging/PackagingInfo.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace zarya {

namespace {

int parseVersionPart(const QString& part)
{
    QString digits;
    for (const QChar ch : part) {
        if (ch.isDigit()) {
            digits.append(ch);
        } else {
            break;
        }
    }
    return digits.isEmpty() ? 0 : digits.toInt();
}

bool isNewerAppVersion(const QString& backupVersion, const QString& currentVersion)
{
    const QStringList backupParts = backupVersion.split(QLatin1Char('.'));
    const QStringList currentParts = currentVersion.split(QLatin1Char('.'));
    const int count = qMax(backupParts.size(), currentParts.size());
    for (int i = 0; i < count; ++i) {
        const int backupPart = i < backupParts.size() ? parseVersionPart(backupParts.at(i)) : 0;
        const int currentPart = i < currentParts.size() ? parseVersionPart(currentParts.at(i)) : 0;
        if (backupPart > currentPart) {
            return true;
        }
        if (backupPart < currentPart) {
            return false;
        }
    }
    return false;
}

} // namespace

BackupValidationResult BackupValidator::validateManifest(const BackupManifest& manifest)
{
    BackupValidationResult result;
    result.valid = true;
    result.canImport = true;

    if (manifest.format != QStringLiteral("zarya-backup")) {
        result.errors.append(QStringLiteral("Unsupported backup format."));
        result.valid = false;
        result.canImport = false;
    }

    if (manifest.formatVersion > BackupManifest::currentFormatVersion) {
        result.errors.append(QStringLiteral(
            "Unsupported backup format version %1. This Zarya version supports up to %2.")
                                 .arg(manifest.formatVersion)
                                 .arg(BackupManifest::currentFormatVersion));
        result.valid = false;
        result.canImport = false;
    }

    if (isNewerAppVersion(manifest.appVersion, PackagingInfo::versionString())) {
        result.warnings.append(QStringLiteral(
            "This backup was created by a newer Zarya version (%1). Import may be incomplete.")
                                 .arg(manifest.appVersion));
    }

    if (manifest.redacted) {
        result.warnings.append(
            QStringLiteral("Backup is redacted; credentials and endpoints cannot be restored."));
    }

    return result;
}

bool BackupValidator::verifyChecksums(const BackupManifest& manifest, const QString& stagingDir,
                                      QStringList* errors)
{
    bool ok = true;
    for (auto it = manifest.checksums.constBegin(); it != manifest.checksums.constEnd(); ++it) {
        const QString relativePath = it.key();
        const QString expected = it.value();
        const QString filePath = QDir(stagingDir).filePath(relativePath);
        if (!QFile::exists(filePath)) {
            if (errors) {
                errors->append(QStringLiteral("Missing file for checksum: %1").arg(relativePath));
            }
            ok = false;
            continue;
        }
        const QString actual = QStringLiteral("sha256:%1").arg(sha256OfFile(filePath));
        if (actual.compare(expected, Qt::CaseInsensitive) != 0) {
            if (errors) {
                errors->append(QStringLiteral("Checksum mismatch: %1").arg(relativePath));
            }
            ok = false;
        }
    }
    return ok;
}

QString BackupValidator::sha256OfFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }
    return QString::fromLatin1(hash.result().toHex());
}

} // namespace zarya
