#include "backup/BackupImporter.h"

#include "backup/BackupArchive.h"
#include "backup/BackupExporter.h"
#include "backup/BackupValidator.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "storage/DnsStore.h"
#include "storage/ProfileStore.h"
#include "storage/RoutingStore.h"
#include "storage/SubscriptionStore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QUuid>

namespace zarya {

namespace {

bool isMachineSpecificSettingsKey(const QString& key)
{
    return key == QStringLiteral("cores/xrayPath") || key == QStringLiteral("cores/singBoxPath")
           || key == QStringLiteral("startup/startAtLogin")
           || key == QStringLiteral("startup/lastStartedProfileId")
           || key == QStringLiteral("proxy/macPreferredNetworkService")
           || key == QStringLiteral("runtime/lastShutdownClean")
           || key == QStringLiteral("runtime/tunWasRunning")
           || key == QStringLiteral("runtime/lastRuntimeMode")
           || key.startsWith(QStringLiteral("window/"));
}

QString uniqueName(const QString& base, const QSet<QString>& used)
{
    if (!used.contains(base)) {
        return base;
    }
    int suffix = 2;
    while (used.contains(QStringLiteral("%1 (imported %2)").arg(base).arg(suffix))) {
        ++suffix;
    }
    return QStringLiteral("%1 (imported %2)").arg(base).arg(suffix);
}

QVector<Profile> loadProfilesFromFile(const QString& filePath)
{
    ProfileStore store(filePath);
    QString error;
    return store.load(&error);
}

QVector<Subscription> loadSubscriptionsFromFile(const QString& filePath)
{
    SubscriptionStore store(filePath);
    QString error;
    return store.load(&error);
}

} // namespace

BackupImporter::BackupImporter(LogCallback log)
    : m_log(std::move(log))
{
}

bool BackupImporter::extractAndLoadManifest(const QString& archivePath, QString* stagingDir,
                                            BackupManifest* manifest, QString* error)
{
    if (m_log) {
        m_log(QStringLiteral("Import backup selected: %1").arg(archivePath));
    }

    const QString staging =
        QDir(QDir::tempPath())
            .filePath(QStringLiteral("zarya-backup-import-%1")
                          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QDir().mkpath(staging);

    const BackupArchiveResult extracted = BackupArchive::extractZip(archivePath, staging);
    if (!extracted.ok) {
        if (error) {
            *error = extracted.error;
        }
        return false;
    }

    const QString manifestPath = QDir(staging).filePath(QStringLiteral("manifest.json"));
    if (!BackupManifestIO::read(manifestPath, manifest, error)) {
        return false;
    }

    if (m_log) {
        m_log(QStringLiteral("Manifest format version: %1").arg(manifest->formatVersion));
    }

    const BackupValidationResult validation = BackupValidator::validateManifest(*manifest);
    if (!validation.valid) {
        if (error) {
            *error = validation.errors.join(QStringLiteral("; "));
        }
        return false;
    }

    QStringList checksumErrors;
    if (!BackupValidator::verifyChecksums(*manifest, staging, &checksumErrors)) {
        if (error) {
            *error = checksumErrors.join(QStringLiteral("; "));
        }
        return false;
    }

    for (auto it = manifest->checksums.constBegin(); it != manifest->checksums.constEnd(); ++it) {
        if (m_log) {
            m_log(QStringLiteral("Checksum verified: %1").arg(it.key()));
        }
    }

    *stagingDir = staging;
    return true;
}

bool BackupImporter::createPreImportBackup(QString* backupPath, QString* error)
{
    QDir().mkpath(AppPaths::backupsDir());
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString path =
        QDir(AppPaths::backupsDir())
            .filePath(QStringLiteral("pre-import-backup-%1.zarya-backup.zip").arg(timestamp));

    BackupExportOptions options;
    for (BackupCategory category : defaultExportCategories()) {
        options.categories.insert(category);
    }
    options.outputPath = path;

    BackupExporter exporter(m_log);
    if (!exporter.exportToArchive(options, error)) {
        return false;
    }

    if (m_log) {
        m_log(QStringLiteral("Pre-import backup created: %1").arg(path));
    }
    *backupPath = path;
    return true;
}

bool BackupImporter::importProfiles(const QString& filePath, ImportMode mode, QString* error)
{
    if (mode == ImportMode::Skip) {
        return true;
    }

    const QVector<Profile> imported = loadProfilesFromFile(filePath);
    ProfileStore store;
    QVector<Profile> current = store.load(error);
    if (!error->isEmpty() && !QFile::exists(store.filePath())) {
        error->clear();
        current.clear();
    }

    if (mode == ImportMode::Replace) {
        if (!store.save(imported, error)) {
            return false;
        }
        if (m_log) {
            m_log(QStringLiteral("Imported profiles: replaced with %1 items").arg(imported.size()));
        }
        return true;
    }

    QHash<QString, int> indexById;
    for (int i = 0; i < current.size(); ++i) {
        indexById.insert(current.at(i).id, i);
    }

    int added = 0;
    int updated = 0;
    int skipped = 0;
    for (const Profile& profile : imported) {
        if (indexById.contains(profile.id)) {
            current[indexById.value(profile.id)] = profile;
            ++updated;
        } else {
            current.append(profile);
            ++added;
        }
    }

    if (!store.save(current, error)) {
        return false;
    }
    if (m_log) {
        m_log(QStringLiteral("Imported profiles: added %1, updated %2, skipped %3")
                  .arg(added)
                  .arg(updated)
                  .arg(skipped));
    }
    return true;
}

bool BackupImporter::importSubscriptions(const QString& filePath, ImportMode mode, QString* error)
{
    if (mode == ImportMode::Skip) {
        return true;
    }

    const QVector<Subscription> imported = loadSubscriptionsFromFile(filePath);
    SubscriptionStore store;
    QVector<Subscription> current = store.load(error);
    if (!error->isEmpty() && !QFile::exists(store.filePath())) {
        error->clear();
        current.clear();
    }

    if (mode == ImportMode::Replace) {
        return store.save(imported, error);
    }

    QHash<QString, int> byId;
    QHash<QString, int> byUrl;
    for (int i = 0; i < current.size(); ++i) {
        byId.insert(current.at(i).id, i);
        byUrl.insert(current.at(i).url.trimmed().toLower(), i);
    }

    for (const Subscription& subscription : imported) {
        if (byId.contains(subscription.id)) {
            current[byId.value(subscription.id)] = subscription;
        } else {
            const QString normalizedUrl = subscription.url.trimmed().toLower();
            if (!normalizedUrl.isEmpty() && byUrl.contains(normalizedUrl)) {
                current[byUrl.value(normalizedUrl)] = subscription;
            } else {
                current.append(subscription);
            }
        }
    }
    return store.save(current, error);
}

bool BackupImporter::importRouting(const QString& filePath, ImportMode mode, QString* error)
{
    if (mode == ImportMode::Skip) {
        return true;
    }

    RoutingStore importStore(filePath);
    QVector<RoutingProfile> imported = importStore.load(error);

    RoutingStore targetStore;
    if (mode == ImportMode::Replace) {
        QVector<RoutingProfile> merged = imported;
        for (const RoutingProfile& builtIn : RoutingProfile::createBuiltInProfiles()) {
            bool found = false;
            for (const RoutingProfile& profile : merged) {
                if (profile.id == builtIn.id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                merged.append(builtIn);
            }
        }
        return targetStore.save(merged, error);
    }

    QVector<RoutingProfile> current = targetStore.load(error);
    QHash<QString, int> byId;
    QSet<QString> names;
    for (int i = 0; i < current.size(); ++i) {
        byId.insert(current.at(i).id, i);
        names.insert(current.at(i).name);
    }

    for (const RoutingProfile& profile : imported) {
        if (profile.isBuiltIn) {
            continue;
        }
        if (byId.contains(profile.id)) {
            current[byId.value(profile.id)] = profile;
        } else {
            RoutingProfile copy = profile;
            if (names.contains(copy.name)) {
                copy.name = uniqueName(copy.name, names);
            }
            names.insert(copy.name);
            current.append(copy);
        }
    }
    return targetStore.save(current, error);
}

bool BackupImporter::importDns(const QString& filePath, ImportMode mode, QString* error)
{
    if (mode == ImportMode::Skip) {
        return true;
    }

    DnsStore importStore(filePath);
    QVector<DnsProfile> imported = importStore.load(error);

    DnsStore targetStore;
    if (mode == ImportMode::Replace) {
        QVector<DnsProfile> merged = imported;
        for (const DnsProfile& builtIn : DnsProfile::createBuiltInProfiles()) {
            bool found = false;
            for (const DnsProfile& profile : merged) {
                if (profile.id == builtIn.id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                merged.append(builtIn);
            }
        }
        return targetStore.save(merged, error);
    }

    QVector<DnsProfile> current = targetStore.load(error);
    QHash<QString, int> byId;
    QSet<QString> names;
    for (int i = 0; i < current.size(); ++i) {
        byId.insert(current.at(i).id, i);
        names.insert(current.at(i).name);
    }

    for (const DnsProfile& profile : imported) {
        if (profile.isBuiltIn) {
            continue;
        }
        if (byId.contains(profile.id)) {
            current[byId.value(profile.id)] = profile;
        } else {
            DnsProfile copy = profile;
            if (names.contains(copy.name)) {
                copy.name = uniqueName(copy.name, names);
            }
            names.insert(copy.name);
            current.append(copy);
        }
    }
    return targetStore.save(current, error);
}

bool BackupImporter::importSettings(const QString& filePath, bool machineSpecific, QString* error)
{
    QSettings imported(filePath, QSettings::IniFormat);
    if (imported.status() != QSettings::NoError) {
        if (error) {
            *error = QStringLiteral("Failed to read imported settings.");
        }
        return false;
    }

    QSettings& target = AppSettings::settings();
    const QStringList keys = imported.allKeys();
    for (const QString& key : keys) {
        if (!machineSpecific && isMachineSpecificSettingsKey(key)) {
            continue;
        }
        target.setValue(key, imported.value(key));
    }
    target.sync();
    return true;
}

bool BackupImporter::importGeoDataSettings(const QString& filePath, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    QSettings& settings = AppSettings::settings();
    settings.setValue(QStringLiteral("geodata/selectedSourceId"),
                      object.value(QStringLiteral("selectedSourceId")));
    settings.setValue(QStringLiteral("geodata/autoCheckOnStartup"),
                      object.value(QStringLiteral("autoCheckOnStartup")));
    settings.setValue(QStringLiteral("geodata/warnIfMissing"),
                      object.value(QStringLiteral("warnIfMissing")));
    if (object.contains(QStringLiteral("verifiedGeosite"))) {
        settings.setValue(QStringLiteral("geodata/verified/geosite"),
                          object.value(QStringLiteral("verifiedGeosite")));
    }
    if (object.contains(QStringLiteral("verifiedGeoip"))) {
        settings.setValue(QStringLiteral("geodata/verified/geoip"),
                          object.value(QStringLiteral("verifiedGeoip")));
    }
    settings.sync();
    return true;
}

bool BackupImporter::importRuleSetMetadata(const QString& filePath, ImportMode mode, QString* error)
{
    if (mode == ImportMode::Skip) {
        return true;
    }
    if (mode == ImportMode::Replace) {
        return QFile::copy(filePath, AppPaths::ruleSetStorePath())
               || QFile::exists(AppPaths::ruleSetStorePath());
    }
    QDir().mkpath(QFileInfo(AppPaths::ruleSetStorePath()).absolutePath());
    if (!QFile::exists(AppPaths::ruleSetStorePath())) {
        return QFile::copy(filePath, AppPaths::ruleSetStorePath());
    }
    return QFile::remove(AppPaths::ruleSetStorePath())
           && QFile::copy(filePath, AppPaths::ruleSetStorePath());
}

bool BackupImporter::importRuleSetFiles(const QString& sourceDir, QString* error)
{
    QDir source(sourceDir);
    if (!source.exists()) {
        return true;
    }
    const QStringList files = source.entryList(QDir::Files);
    if (files.isEmpty()) {
        return true;
    }

    QDir dest(AppPaths::singBoxRuleSetDir());
    dest.mkpath(QStringLiteral("."));
    for (const QString& file : files) {
        const QString target = dest.filePath(file);
        if (QFile::exists(target)) {
            QFile::remove(target);
        }
        if (!QFile::copy(source.filePath(file), target)) {
            if (error) {
                *error = QStringLiteral("Failed to import rule-set file %1").arg(file);
            }
            return false;
        }
    }
    return true;
}

bool BackupImporter::importGeoFiles(const QString& sourceDir, QString* error)
{
    const QString destDir = AppPaths::xrayResourceDir();
    QDir().mkpath(destDir);
    for (const QString& name : {QStringLiteral("geoip.dat"), QStringLiteral("geosite.dat")}) {
        const QString source = QDir(sourceDir).filePath(name);
        if (!QFile::exists(source)) {
            continue;
        }
        const QString target = QDir(destDir).filePath(name);
        if (QFile::exists(target)) {
            QFile::remove(target);
        }
        if (!QFile::copy(source, target)) {
            if (error) {
                *error = QStringLiteral("Failed to import %1").arg(name);
            }
            return false;
        }
    }
    return true;
}

bool BackupImporter::importCoreMetadata(const QString& filePath, QString* error)
{
    Q_UNUSED(error);
    Q_UNUSED(filePath);
    return true;
}

BackupImportResult BackupImporter::importFromStaging(const BackupImportOptions& options,
                                                     const BackupManifest& manifest,
                                                     QString* error)
{
    BackupImportResult result;

    if (!createPreImportBackup(&result.preImportBackupPath, error)) {
        result.message = error ? *error : QStringLiteral("Pre-import backup failed.");
        return result;
    }

    const QString stagingDir = options.stagingDir;
    const auto modeFor = [&](BackupCategory category) {
        return options.categoryModes.value(category, ImportMode::Skip);
    };

    const auto importIfIncluded = [&](BackupCategory category, auto&& importer) {
        const BackupCategoryEntry entry = manifest.categories.value(backupCategoryKey(category));
        if (!entry.included || entry.file.isEmpty()) {
            return true;
        }
        const ImportMode mode = modeFor(category);
        if (mode == ImportMode::Skip) {
            return true;
        }
        if (m_log) {
            m_log(QStringLiteral("Importing %1: %2")
                      .arg(backupCategoryDisplayName(category),
                           mode == ImportMode::Replace ? QStringLiteral("replace")
                                                       : QStringLiteral("merge")));
        }
        return importer();
    };

    bool ok = true;
    ok = importIfIncluded(BackupCategory::Profiles, [&]() {
             return importProfiles(QDir(stagingDir).filePath(QStringLiteral("data/profiles.json")),
                                   modeFor(BackupCategory::Profiles), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::Subscriptions, [&]() {
             return importSubscriptions(
                 QDir(stagingDir).filePath(QStringLiteral("data/subscriptions.json")),
                 modeFor(BackupCategory::Subscriptions), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::RoutingProfiles, [&]() {
             return importRouting(QDir(stagingDir).filePath(QStringLiteral("data/routing.json")),
                                  modeFor(BackupCategory::RoutingProfiles), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::DnsProfiles, [&]() {
             return importDns(QDir(stagingDir).filePath(QStringLiteral("data/dns.json")),
                              modeFor(BackupCategory::DnsProfiles), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::AppSettings, [&]() {
             return importSettings(QDir(stagingDir).filePath(QStringLiteral("data/settings.ini")),
                                 options.importMachineSpecificSettings, error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::GeoDataSettings, [&]() {
             return importGeoDataSettings(
                 QDir(stagingDir).filePath(QStringLiteral("data/geodata-settings.json")), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::SingBoxRuleSetMetadata, [&]() {
             return importRuleSetMetadata(
                 QDir(stagingDir).filePath(QStringLiteral("data/rulesets.json")),
                 modeFor(BackupCategory::SingBoxRuleSetMetadata), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::SingBoxRuleSetFiles, [&]() {
             return importRuleSetFiles(
                 QDir(stagingDir).filePath(QStringLiteral("optional/rule-set")), error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::XrayGeoDataFiles, [&]() {
             return importGeoFiles(QDir(stagingDir).filePath(QStringLiteral("optional/geo")),
                                  error);
         })
         && ok;
    ok = importIfIncluded(BackupCategory::CoreMetadata, [&]() {
             return importCoreMetadata(
                 QDir(stagingDir).filePath(QStringLiteral("data/core-metadata.json")), error);
         })
         && ok;

    result.ok = ok;
    if (ok) {
        result.message = QStringLiteral("Import completed.");
        if (m_log) {
            m_log(QStringLiteral("Import completed"));
        }
    } else {
        result.message = error ? *error : QStringLiteral("Import failed.");
    }
    return result;
}

} // namespace zarya
