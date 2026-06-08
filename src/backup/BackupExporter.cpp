#include "backup/BackupExporter.h"

#include "backup/BackupArchive.h"
#include "backup/BackupRedactor.h"
#include "backup/BackupValidator.h"
#include "cores/CorePaths.h"
#include "domain/CoreType.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QFileInfo>
#include <QTemporaryDir>

namespace zarya {

namespace {

bool copyFile(const QString& source, const QString& destination, QString* error)
{
    QDir().mkpath(QFileInfo(destination).absolutePath());
    if (!QFile::exists(source)) {
        return false;
    }
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

bool writeEmptyJsonArray(const QString& destination, QString* error)
{
    QDir().mkpath(QFileInfo(destination).absolutePath());
    QFile file(destination);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write("[]");
    return true;
}

bool copyOrEmptyJsonArray(const QString& source, const QString& destination, QString* error)
{
    if (!QFile::exists(source)) {
        return writeEmptyJsonArray(destination, error);
    }
    return copyFile(source, destination, error);
}

bool copyDirectoryFiles(const QString& sourceDir, const QString& destinationDir, QString* error)
{
    QDir source(sourceDir);
    if (!source.exists()) {
        return true;
    }
    QDir().mkpath(destinationDir);
    const QStringList files = source.entryList(QDir::Files);
    for (const QString& file : files) {
        if (!copyFile(source.filePath(file), QDir(destinationDir).filePath(file), error)) {
            return false;
        }
    }
    return true;
}

bool isSettingsKeyExcludedFromExport(const QString& key)
{
    return key == QStringLiteral("runtime/lastShutdownClean")
           || key == QStringLiteral("runtime/tunWasRunning")
           || key == QStringLiteral("runtime/lastRuntimeMode");
}

bool exportSettingsIni(const QString& destinationPath, QString* error)
{
    QSettings exportSettings(destinationPath, QSettings::IniFormat);
    const QSettings& source = AppSettings::settings();
    const QStringList keys = source.allKeys();
    for (const QString& key : keys) {
        if (isSettingsKeyExcludedFromExport(key)) {
            continue;
        }
        exportSettings.setValue(key, source.value(key));
    }
    exportSettings.sync();
    if (exportSettings.status() != QSettings::NoError) {
        if (error) {
            *error = QStringLiteral("Failed to export settings.");
        }
        return false;
    }
    return true;
}

QJsonObject buildGeoDataSettingsJson()
{
    const QSettings& settings = AppSettings::settings();
    QJsonObject object;
    object.insert(QStringLiteral("selectedSourceId"),
                  settings.value(QStringLiteral("geodata/selectedSourceId"), QStringLiteral("loyalsoldier"))
                      .toString());
    object.insert(QStringLiteral("autoCheckOnStartup"),
                  settings.value(QStringLiteral("geodata/autoCheckOnStartup"), false).toBool());
    object.insert(QStringLiteral("warnIfMissing"),
                  settings.value(QStringLiteral("geodata/warnIfMissing"), true).toBool());
    object.insert(QStringLiteral("verifiedGeosite"),
                  settings.value(QStringLiteral("geodata/verified/geosite")).toString());
    object.insert(QStringLiteral("verifiedGeoip"),
                  settings.value(QStringLiteral("geodata/verified/geoip")).toString());
    return object;
}

QJsonObject buildCoreMetadataJson()
{
    QJsonObject root;
    const auto readMeta = [](CoreType type) {
        const QString installDir = CorePaths::managedInstallDir(type);
        const QString metaPath = CorePaths::metadataFilePath(installDir);
        QFile file(metaPath);
        if (!file.open(QIODevice::ReadOnly)) {
            return QJsonObject{};
        }
        return QJsonDocument::fromJson(file.readAll()).object();
    };
    root.insert(QStringLiteral("xray"), readMeta(CoreType::Xray));
    root.insert(QStringLiteral("singBox"), readMeta(CoreType::SingBox));
    return root;
}

QJsonObject buildAppInfoJson()
{
    QJsonObject object;
    object.insert(QStringLiteral("appVersion"), PackagingInfo::versionString());
    object.insert(QStringLiteral("platform"), PackagingInfo::platformName());
    object.insert(QStringLiteral("portableMode"), AppPaths::isPortableMode());
    return object;
}

QJsonObject buildPathsJson()
{
    QJsonObject object;
    object.insert(QStringLiteral("dataDir"), AppPaths::dataDir());
    object.insert(QStringLiteral("runtimeDir"), AppPaths::runtimeDir());
    object.insert(QStringLiteral("coresDir"), AppPaths::coresDir());
    return object;
}

} // namespace

BackupExporter::BackupExporter(LogCallback log)
    : m_log(std::move(log))
{
}

QString BackupExporter::categoryArchivePath(BackupCategory category) const
{
    switch (category) {
    case BackupCategory::Profiles:
        return QStringLiteral("data/profiles.json");
    case BackupCategory::Subscriptions:
        return QStringLiteral("data/subscriptions.json");
    case BackupCategory::RoutingProfiles:
        return QStringLiteral("data/routing.json");
    case BackupCategory::DnsProfiles:
        return QStringLiteral("data/dns.json");
    case BackupCategory::AppSettings:
        return QStringLiteral("data/settings.ini");
    case BackupCategory::GeoDataSettings:
        return QStringLiteral("data/geodata-settings.json");
    case BackupCategory::SingBoxRuleSetMetadata:
        return QStringLiteral("data/rulesets.json");
    case BackupCategory::SingBoxRuleSetFiles:
        return QStringLiteral("optional/rule-set/");
    case BackupCategory::XrayGeoDataFiles:
        return QStringLiteral("optional/geo/");
    case BackupCategory::CoreMetadata:
        return QStringLiteral("data/core-metadata.json");
    case BackupCategory::LogsRedacted:
        return QStringLiteral("diagnostics/logs-redacted.txt");
    }
    return {};
}

bool BackupExporter::stageCategory(BackupCategory category, const QString& stagingDir,
                                     BackupRedactionMode redactionMode, RedactionReport* report,
                                     QString* error)
{
    const QString dataDir = QDir(stagingDir).filePath(QStringLiteral("data"));
    QDir().mkpath(dataDir);

    switch (category) {
    case BackupCategory::Profiles:
        return copyOrEmptyJsonArray(AppPaths::profilesFilePath(),
                                    QDir(dataDir).filePath(QStringLiteral("profiles.json")), error);
    case BackupCategory::Subscriptions:
        return copyOrEmptyJsonArray(AppPaths::subscriptionsFilePath(),
                                    QDir(dataDir).filePath(QStringLiteral("subscriptions.json")),
                                    error);
    case BackupCategory::RoutingProfiles:
        return copyOrEmptyJsonArray(AppPaths::routingFilePath(),
                                     QDir(dataDir).filePath(QStringLiteral("routing.json")), error);
    case BackupCategory::DnsProfiles:
        return copyOrEmptyJsonArray(AppPaths::dnsFilePath(),
                                    QDir(dataDir).filePath(QStringLiteral("dns.json")), error);
    case BackupCategory::AppSettings:
        return exportSettingsIni(QDir(dataDir).filePath(QStringLiteral("settings.ini")), error);
    case BackupCategory::GeoDataSettings: {
        QFile file(QDir(dataDir).filePath(QStringLiteral("geodata-settings.json")));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) {
                *error = file.errorString();
            }
            return false;
        }
        file.write(QJsonDocument(buildGeoDataSettingsJson()).toJson(QJsonDocument::Indented));
        return true;
    }
    case BackupCategory::SingBoxRuleSetMetadata:
        return copyFile(AppPaths::ruleSetStorePath(),
                        QDir(dataDir).filePath(QStringLiteral("rulesets.json")), error);
    case BackupCategory::SingBoxRuleSetFiles:
        return copyDirectoryFiles(AppPaths::singBoxRuleSetDir(),
                                  QDir(stagingDir).filePath(QStringLiteral("optional/rule-set")),
                                  error);
    case BackupCategory::XrayGeoDataFiles: {
        const QString geoDir = QDir(stagingDir).filePath(QStringLiteral("optional/geo"));
        QDir().mkpath(geoDir);
        const QString resourceDir = AppPaths::xrayResourceDir();
        bool ok = true;
        for (const QString& name : {QStringLiteral("geoip.dat"), QStringLiteral("geosite.dat")}) {
            ok = copyFile(QDir(resourceDir).filePath(name), QDir(geoDir).filePath(name), error)
                 && ok;
        }
        return ok;
    }
    case BackupCategory::CoreMetadata: {
        QFile file(QDir(dataDir).filePath(QStringLiteral("core-metadata.json")));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) {
                *error = file.errorString();
            }
            return false;
        }
        file.write(QJsonDocument(buildCoreMetadataJson()).toJson(QJsonDocument::Indented));
        return true;
    }
    case BackupCategory::LogsRedacted:
        return true;
    }
    return false;
}

void BackupExporter::addChecksums(const QString& stagingDir, BackupManifest* manifest)
{
    for (auto it = manifest->categories.constBegin(); it != manifest->categories.constEnd(); ++it) {
        const BackupCategoryEntry& entry = it.value();
        if (!entry.included || entry.file.isEmpty() || entry.file.endsWith(QLatin1Char('/'))) {
            continue;
        }
        const QString filePath = QDir(stagingDir).filePath(entry.file);
        if (QFile::exists(filePath)) {
            manifest->checksums.insert(
                entry.file, QStringLiteral("sha256:%1").arg(BackupValidator::sha256OfFile(filePath)));
        }
    }
}

bool BackupExporter::exportToArchive(const BackupExportOptions& options, QString* error)
{
    if (options.outputPath.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Output path is required.");
        }
        return false;
    }

    if (m_log) {
        m_log(QStringLiteral("Export backup started"));
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        if (error) {
            *error = QStringLiteral("Failed to create temporary staging directory.");
        }
        return false;
    }

    const QString stagingDir = tempDir.path();
    QDir().mkpath(QDir(stagingDir).filePath(QStringLiteral("diagnostics")));

    BackupRedactionMode redactionMode = options.redactionMode;
    if (options.diagnosticBackup) {
        redactionMode = BackupRedactionMode::Strict;
    }

    RedactionReport report;
    BackupManifest manifest;
    manifest.formatVersion = BackupManifest::currentFormatVersion;
    manifest.appVersion = PackagingInfo::versionString();
    manifest.createdAt = QDateTime::currentDateTimeUtc();
    manifest.platform = PackagingInfo::platformName();
    manifest.portableMode = AppPaths::isPortableMode();
    manifest.redacted = redactionMode != BackupRedactionMode::None;
    if (manifest.redacted) {
        manifest.redactionMode =
            redactionMode == BackupRedactionMode::Strict ? QStringLiteral("strict")
                                                         : QStringLiteral("basic");
    }

    QSet<BackupCategory> categories = options.categories;
    if (categories.isEmpty()) {
        for (BackupCategory category : defaultExportCategories()) {
            categories.insert(category);
        }
    }

    for (BackupCategory category : categories) {
        if (m_log) {
            m_log(QStringLiteral("Backup category included: %1")
                      .arg(backupCategoryDisplayName(category)));
        }
        if (!stageCategory(category, stagingDir, redactionMode, &report, error)) {
            if (category == BackupCategory::SingBoxRuleSetMetadata
                && !QFile::exists(AppPaths::ruleSetStorePath())) {
                continue;
            }
            return false;
        }

        BackupCategoryEntry entry;
        entry.included = true;
        entry.file = categoryArchivePath(category);
        entry.redacted = manifest.redacted
                         && (category == BackupCategory::Profiles
                             || category == BackupCategory::Subscriptions
                             || category == BackupCategory::AppSettings);
        if (!entry.file.endsWith(QLatin1Char('/'))) {
            const QString stagedFile = QDir(stagingDir).filePath(entry.file);
            if (QFile::exists(stagedFile)) {
                entry.count = BackupManifestIO::countItemsInJsonFile(stagedFile);
            }
        }
        manifest.categories.insert(backupCategoryKey(category), entry);

        if (manifest.redacted && !entry.file.isEmpty() && !entry.file.endsWith(QLatin1Char('/'))) {
            const QString stagedFile = QDir(stagingDir).filePath(entry.file);
            if (category == BackupCategory::AppSettings) {
                QSettings settings(stagedFile, QSettings::IniFormat);
                for (const QString& key :
                     {QStringLiteral("cores/xrayPath"), QStringLiteral("cores/singBoxPath")}) {
                    const QString path = settings.value(key).toString();
                    if (!path.isEmpty()) {
                        settings.setValue(key, QFileInfo(path).fileName());
                        report.redactedFields.append(QStringLiteral("settings.%1").arg(key));
                    }
                }
                settings.sync();
            } else if (!BackupRedactor::redactFile(stagedFile, redactionMode, &report, error)) {
                return false;
            }
        }
    }

    if (manifest.redacted) {
        if (m_log) {
            m_log(QStringLiteral("Backup redaction mode: %1").arg(manifest.redactionMode));
        }
        const QString reportPath =
            QDir(stagingDir).filePath(QStringLiteral("diagnostics/redaction-report.json"));
        if (!BackupRedactor::writeRedactionReport(reportPath, report, error)) {
            return false;
        }
    }

    QFileInfo appInfoPath(QDir(stagingDir).filePath(QStringLiteral("diagnostics/app-info.json")));
    QFile appInfoFile(appInfoPath.absoluteFilePath());
    if (appInfoFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        appInfoFile.write(QJsonDocument(buildAppInfoJson()).toJson(QJsonDocument::Indented));
    }
    QFile pathsFile(QDir(stagingDir).filePath(QStringLiteral("diagnostics/paths.json")));
    if (pathsFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        pathsFile.write(QJsonDocument(buildPathsJson()).toJson(QJsonDocument::Indented));
    }

    addChecksums(stagingDir, &manifest);

    if (m_log) {
        m_log(QStringLiteral("Writing manifest"));
    }
    if (!BackupManifestIO::write(manifest, QDir(stagingDir).filePath(QStringLiteral("manifest.json")),
                                 error)) {
        return false;
    }

    const BackupArchiveResult archived = BackupArchive::createZip(stagingDir, options.outputPath);
    if (!archived.ok) {
        if (error) {
            *error = archived.error;
        }
        return false;
    }

    if (m_log) {
        m_log(QStringLiteral("Backup archive created: %1").arg(options.outputPath));
    }
    return true;
}

} // namespace zarya
