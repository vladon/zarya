#include "diagnostics/DiagnosticsBundleWriter.h"

#include "backup/BackupArchive.h"
#include "backup/BackupValidator.h"
#include "diagnostics/DiagnosticsCategory.h"
#include "diagnostics/DiagnosticsManifest.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QJsonDocument>
#include <QTemporaryDir>

namespace zarya {

bool DiagnosticsBundleWriter::write(const DiagnosticsSnapshot& snapshot,
                                    const DiagnosticsOptions& options, const QString& outputPath,
                                    QString* error)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        if (error) {
            *error = QStringLiteral("Failed to create diagnostics staging directory.");
        }
        return false;
    }

    const QString stagingDir = tempDir.path();
    for (auto it = snapshot.jsonFiles.constBegin(); it != snapshot.jsonFiles.constEnd(); ++it) {
        const QString filePath = QDir(stagingDir).filePath(it.key());
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) {
                *error = file.errorString();
            }
            return false;
        }
        file.write(QJsonDocument(it.value()).toJson(QJsonDocument::Indented));
    }

    for (auto it = snapshot.textFiles.constBegin(); it != snapshot.textFiles.constEnd(); ++it) {
        const QString filePath = QDir(stagingDir).filePath(it.key());
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) {
                *error = file.errorString();
            }
            return false;
        }
        file.write(it.value().toUtf8());
    }

    DiagnosticsManifest manifest;
    manifest.appVersion = PackagingInfo::versionString();
    manifest.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    manifest.platform = PackagingInfo::platformName();
    manifest.portableMode = AppPaths::isPortableMode();
    manifest.redactionMode =
        options.redactionMode == DiagnosticsRedactionMode::Strict ? QStringLiteral("strict")
                                                                  : QStringLiteral("basic");
    manifest.secretsIncluded = false;
    manifest.warnings = snapshot.warnings;

    QSet<DiagnosticsCategory> categories = options.categories;
    if (categories.isEmpty()) {
        for (DiagnosticsCategory category : defaultDiagnosticsCategories()) {
            categories.insert(category);
        }
    }
    for (DiagnosticsCategory category : categories) {
        manifest.categories.insert(diagnosticsCategoryKey(category), true);
    }

    const auto addChecksum = [&](const QString& relativePath) {
        const QString filePath = QDir(stagingDir).filePath(relativePath);
        if (QFile::exists(filePath)) {
            manifest.checksums.insert(
                relativePath,
                QStringLiteral("sha256:%1").arg(BackupValidator::sha256OfFile(filePath)));
        }
    };

    for (auto it = snapshot.jsonFiles.constBegin(); it != snapshot.jsonFiles.constEnd(); ++it) {
        addChecksum(it.key());
    }
    for (auto it = snapshot.textFiles.constBegin(); it != snapshot.textFiles.constEnd(); ++it) {
        addChecksum(it.key());
    }

    if (!DiagnosticsManifestIO::write(manifest, QDir(stagingDir).filePath(QStringLiteral("manifest.json")),
                                      error)) {
        return false;
    }
    addChecksum(QStringLiteral("manifest.json"));

    QFileInfo outputInfo(outputPath);
    QDir().mkpath(outputInfo.absolutePath());

    const BackupArchiveResult archived = BackupArchive::createZip(stagingDir, outputPath);
    if (!archived.ok) {
        if (error) {
            *error = archived.error;
        }
        return false;
    }
    return true;
}

} // namespace zarya
