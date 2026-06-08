#include "backup/BackupManifest.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

QJsonObject categoryEntryToJson(const BackupCategoryEntry& entry)
{
    QJsonObject object;
    object.insert(QStringLiteral("included"), entry.included);
    if (!entry.file.isEmpty()) {
        object.insert(QStringLiteral("file"), entry.file);
    }
    if (entry.count >= 0) {
        object.insert(QStringLiteral("count"), entry.count);
    }
    object.insert(QStringLiteral("redacted"), entry.redacted);
    return object;
}

BackupCategoryEntry categoryEntryFromJson(const QJsonObject& object)
{
    BackupCategoryEntry entry;
    entry.included = object.value(QStringLiteral("included")).toBool();
    entry.file = object.value(QStringLiteral("file")).toString();
    entry.count = object.contains(QStringLiteral("count")) ? object.value(QStringLiteral("count")).toInt()
                                                           : -1;
    entry.redacted = object.value(QStringLiteral("redacted")).toBool();
    return entry;
}

} // namespace

bool BackupManifestIO::write(const BackupManifest& manifest, const QString& filePath,
                             QString* error)
{
    QJsonObject root;
    root.insert(QStringLiteral("format"), manifest.format);
    root.insert(QStringLiteral("formatVersion"), manifest.formatVersion);
    root.insert(QStringLiteral("appName"), manifest.appName);
    root.insert(QStringLiteral("appVersion"), manifest.appVersion);
    root.insert(QStringLiteral("createdAt"), manifest.createdAt.toUTC().toString(Qt::ISODate));
    root.insert(QStringLiteral("createdBy"), manifest.createdBy);
    root.insert(QStringLiteral("platform"), manifest.platform);
    root.insert(QStringLiteral("portableMode"), manifest.portableMode);
    root.insert(QStringLiteral("redacted"), manifest.redacted);
    if (!manifest.redactionMode.isEmpty()) {
        root.insert(QStringLiteral("redactionMode"), manifest.redactionMode);
    }

    QJsonObject categories;
    for (auto it = manifest.categories.constBegin(); it != manifest.categories.constEnd(); ++it) {
        categories.insert(it.key(), categoryEntryToJson(it.value()));
    }
    root.insert(QStringLiteral("categories"), categories);

    QJsonArray warnings;
    for (const QString& warning : manifest.warnings) {
        warnings.append(warning);
    }
    root.insert(QStringLiteral("warnings"), warnings);

    QJsonObject checksums;
    for (auto it = manifest.checksums.constBegin(); it != manifest.checksums.constEnd(); ++it) {
        checksums.insert(it.key(), it.value());
    }
    root.insert(QStringLiteral("checksums"), checksums);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool BackupManifestIO::read(const QString& filePath, BackupManifest* manifest, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = QStringLiteral("Invalid manifest JSON: %1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = document.object();
    manifest->format = root.value(QStringLiteral("format")).toString();
    manifest->formatVersion = root.value(QStringLiteral("formatVersion")).toInt();
    manifest->appName = root.value(QStringLiteral("appName")).toString();
    manifest->appVersion = root.value(QStringLiteral("appVersion")).toString();
    manifest->createdAt =
        QDateTime::fromString(root.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    manifest->createdBy = root.value(QStringLiteral("createdBy")).toString();
    manifest->platform = root.value(QStringLiteral("platform")).toString();
    manifest->portableMode = root.value(QStringLiteral("portableMode")).toBool();
    manifest->redacted = root.value(QStringLiteral("redacted")).toBool();
    manifest->redactionMode = root.value(QStringLiteral("redactionMode")).toString();

    manifest->categories.clear();
    const QJsonObject categories = root.value(QStringLiteral("categories")).toObject();
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it) {
        manifest->categories.insert(it.key(), categoryEntryFromJson(it.value().toObject()));
    }

    manifest->warnings.clear();
    const QJsonArray warnings = root.value(QStringLiteral("warnings")).toArray();
    for (const QJsonValue& value : warnings) {
        manifest->warnings.append(value.toString());
    }

    manifest->checksums.clear();
    const QJsonObject checksums = root.value(QStringLiteral("checksums")).toObject();
    for (auto it = checksums.constBegin(); it != checksums.constEnd(); ++it) {
        manifest->checksums.insert(it.key(), it.value().toString());
    }
    return true;
}

int BackupManifestIO::countItemsInJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return -1;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (document.isArray()) {
        return document.array().size();
    }
    if (document.isObject()) {
        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("profiles"))) {
            return object.value(QStringLiteral("profiles")).toArray().size();
        }
        if (object.contains(QStringLiteral("items"))) {
            return object.value(QStringLiteral("items")).toArray().size();
        }
        if (object.contains(QStringLiteral("custom"))) {
            return object.value(QStringLiteral("custom")).toArray().size();
        }
    }
    return -1;
}

} // namespace zarya
