#include "backup/BackupArchive.h"

#include "cores/CoreArchiveExtractor.h"

#include <QDir>
#include <QProcess>

namespace zarya {

BackupArchiveResult BackupArchive::createZip(const QString& sourceDir, const QString& outputZipPath)
{
    BackupArchiveResult result;
    QFileInfo outputInfo(outputZipPath);
    QDir().mkpath(outputInfo.absolutePath());

    QProcess process;
    process.setProgram(QStringLiteral("tar"));
    process.setArguments({QStringLiteral("-a"), QStringLiteral("-cf"), outputZipPath,
                          QStringLiteral("-C"), sourceDir, QStringLiteral(".")});
    process.start();
    if (!process.waitForFinished(300000)) {
        process.kill();
        result.error = QStringLiteral("Backup archive creation timed out.");
        return result;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        result.error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (result.error.isEmpty()) {
            result.error = QStringLiteral("Failed to create backup archive.");
        }
        return result;
    }
    result.ok = true;
    return result;
}

BackupArchiveResult BackupArchive::extractZip(const QString& zipPath, const QString& destinationDir)
{
    BackupArchiveResult result;
    QString validationError;
    if (!validateZipEntries(zipPath, &validationError)) {
        result.error = validationError;
        return result;
    }

    const ExtractResult extracted = CoreArchiveExtractor::extract(zipPath, destinationDir);
    result.ok = extracted.ok;
    result.error = extracted.error;
    return result;
}

bool BackupArchive::validateZipEntries(const QString& zipPath, QString* errorMessage)
{
    return CoreArchiveExtractor::validateArchiveEntries(zipPath, errorMessage);
}

} // namespace zarya
