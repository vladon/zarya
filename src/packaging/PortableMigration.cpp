#include "packaging/PortableMigration.h"

#include "backup/BackupArchive.h"
#include "backup/BackupCategory.h"
#include "backup/BackupManifest.h"
#include "packaging/PackagingInfo.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace zarya {

namespace {

bool copyIfExists(const QString& source, const QString& destination, QString* error)
{
    if (!QFile::exists(source)) {
        return true;
    }
    QDir().mkpath(QFileInfo(destination).absolutePath());
    if (QFile::exists(destination)) {
        QFile::remove(destination);
    }
    if (!QFile::copy(source, destination)) {
        if (error) {
            *error = QStringLiteral("Failed to copy %1").arg(source);
        }
        return false;
    }
    return true;
}

} // namespace

QString PortableMigration::portableDataDir(const QString& portableRoot)
{
    const QDir root(portableRoot);
    const QString dataPath = root.filePath(QStringLiteral("data"));
    if (QDir(dataPath).exists()) {
        return dataPath;
    }
    return portableRoot;
}

PortableDataPreview PortableMigration::preview(const QString& portableRoot)
{
    PortableDataPreview result;
    result.folder = portableRoot;
    const QDir root(portableRoot);
    if (!root.exists()) {
        return result;
    }

    result.hasPortableFlag = QFile::exists(root.filePath(QStringLiteral("portable.flag")));
    const QString dataDir = portableDataDir(portableRoot);

    const QString profilesPath = QDir(dataDir).filePath(QStringLiteral("profiles.json"));
    const QString subscriptionsPath = QDir(dataDir).filePath(QStringLiteral("subscriptions.json"));
    result.hasRouting = QFile::exists(QDir(dataDir).filePath(QStringLiteral("routing.json")));
    result.hasDns = QFile::exists(QDir(dataDir).filePath(QStringLiteral("dns.json")));
    result.hasSettings = QFile::exists(QDir(dataDir).filePath(QStringLiteral("settings.ini")));

    if (QFile::exists(profilesPath)) {
        result.profileCount = BackupManifestIO::countItemsInJsonFile(profilesPath);
    }
    if (QFile::exists(subscriptionsPath)) {
        result.subscriptionCount = BackupManifestIO::countItemsInJsonFile(subscriptionsPath);
    }

    result.valid = result.hasPortableFlag
                   || QFile::exists(profilesPath)
                   || QFile::exists(subscriptionsPath)
                   || result.hasRouting
                   || result.hasDns
                   || result.hasSettings;
    return result;
}

bool PortableMigration::createBackupArchive(const QString& portableRoot,
                                            const QString& outputZipPath, QString* error)
{
    const PortableDataPreview dataPreview = preview(portableRoot);
    if (!dataPreview.valid) {
        if (error) {
            *error = QStringLiteral("Selected folder does not contain portable Zarya data.");
        }
        return false;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        if (error) {
            *error = QStringLiteral("Could not create temporary directory.");
        }
        return false;
    }

    const QString stagingDir = tempDir.path();
    const QString dataDir = portableDataDir(portableRoot);

    const struct {
        QString sourceName;
        QString destName;
        BackupCategory category;
    } files[] = {
        {QStringLiteral("profiles.json"), QStringLiteral("profiles.json"), BackupCategory::Profiles},
        {QStringLiteral("subscriptions.json"), QStringLiteral("subscriptions.json"),
         BackupCategory::Subscriptions},
        {QStringLiteral("routing.json"), QStringLiteral("routing.json"),
         BackupCategory::RoutingProfiles},
        {QStringLiteral("dns.json"), QStringLiteral("dns.json"), BackupCategory::DnsProfiles},
        {QStringLiteral("settings.ini"), QStringLiteral("settings.ini"), BackupCategory::AppSettings},
        {QStringLiteral("geodata-settings.json"), QStringLiteral("geodata-settings.json"),
         BackupCategory::GeoDataSettings},
    };

    BackupManifest manifest;
    manifest.formatVersion = BackupManifest::currentFormatVersion;
    manifest.appVersion = PackagingInfo::versionString();
    manifest.createdAt = QDateTime::currentDateTimeUtc();
    manifest.platform = PackagingInfo::platformName();
    manifest.portableMode = true;
    manifest.redacted = false;
    manifest.warnings.append(
        QStringLiteral("Created from portable folder migration preview. Original folder was not "
                         "modified."));

    for (const auto& file : files) {
        const QString source = QDir(dataDir).filePath(file.sourceName);
        if (!QFile::exists(source)) {
            continue;
        }
        const QString destination = QDir(stagingDir).filePath(file.destName);
        if (!copyIfExists(source, destination, error)) {
            return false;
        }

        BackupCategoryEntry entry;
        entry.included = true;
        entry.file = file.destName;
        entry.count = BackupManifestIO::countItemsInJsonFile(destination);
        manifest.categories.insert(backupCategoryKey(file.category), entry);
    }

    if (manifest.categories.isEmpty()) {
        if (error) {
            *error = QStringLiteral("No portable data files were found to import.");
        }
        return false;
    }

    if (!BackupManifestIO::write(manifest, QDir(stagingDir).filePath(QStringLiteral("manifest.json")),
                                 error)) {
        return false;
    }

    const BackupArchiveResult archived = BackupArchive::createZip(stagingDir, outputZipPath);
    if (!archived.ok) {
        if (error) {
            *error = archived.error;
        }
        return false;
    }
    return true;
}

} // namespace zarya
