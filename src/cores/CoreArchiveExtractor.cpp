#include "cores/CoreArchiveExtractor.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

namespace zarya {

bool CoreArchiveExtractor::hasUnsafePath(const QString& entry)
{
    const QString normalized = QDir::fromNativeSeparators(entry);
    if (normalized.startsWith(QLatin1Char('/')) || normalized.contains(QLatin1Char(':'))) {
        return true;
    }
    if (normalized.contains(QStringLiteral("../")) || normalized.startsWith(QStringLiteral(".."))) {
        return true;
    }
    return false;
}

QStringList CoreArchiveExtractor::listArchiveEntries(const QString& archivePath,
                                                     QString* errorMessage)
{
    QProcess process;
    process.setProgram(QStringLiteral("tar"));
    process.setArguments({QStringLiteral("-tf"), archivePath});
    process.start();
    if (!process.waitForFinished(120000)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to list archive entries.");
        }
        return {};
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(process.readAllStandardError()).trimmed();
        }
        return {};
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    return output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
}

bool CoreArchiveExtractor::validateArchiveEntries(const QString& archivePath,
                                                  QString* errorMessage)
{
    const QStringList entries = listArchiveEntries(archivePath, errorMessage);
    if (entries.isEmpty() && errorMessage && !errorMessage->isEmpty()) {
        return false;
    }
    for (const QString& entry : entries) {
        if (hasUnsafePath(entry)) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("Archive contains unsafe path entry: %1").arg(entry);
            }
            return false;
        }
    }
    return true;
}

ExtractResult CoreArchiveExtractor::extract(const QString& archivePath,
                                            const QString& destinationDir)
{
    ExtractResult result;
    result.extractedDir = destinationDir;

    QString validationError;
    if (!validateArchiveEntries(archivePath, &validationError)) {
        result.error = validationError.isEmpty()
                           ? QStringLiteral("Archive validation failed.")
                           : validationError;
        return result;
    }

    QDir().mkpath(destinationDir);

    QProcess process;
    process.setProgram(QStringLiteral("tar"));
    process.setArguments({QStringLiteral("-xf"), archivePath, QStringLiteral("-C"), destinationDir});
    process.start();
    if (!process.waitForFinished(300000)) {
        process.kill();
        result.error = QStringLiteral("Archive extraction timed out.");
        return result;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        result.error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (result.error.isEmpty()) {
            result.error = QStringLiteral("Archive extraction failed.");
        }
        return result;
    }

    const QStringList entries = listArchiveEntries(archivePath, nullptr);
    result.files = entries;
    result.ok = true;
    return result;
}

} // namespace zarya
