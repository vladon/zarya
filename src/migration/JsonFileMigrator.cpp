#include "migration/JsonFileMigrator.h"

#include "packaging/PackagingInfo.h"
#include "storage/SafeJsonWriter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

namespace zarya {

int JsonFileMigrator::readSchemaVersion(const QJsonDocument& document)
{
    if (document.isObject()) {
        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("schemaVersion"))) {
            return object.value(QStringLiteral("schemaVersion")).toInt();
        }
    }
    return 0;
}

void JsonFileMigrator::wrapWithSchema(QJsonObject* object)
{
    object->insert(QStringLiteral("schemaVersion"), currentSchemaVersion);
    object->insert(QStringLiteral("appVersion"), PackagingInfo::versionString());
}

bool JsonFileMigrator::migrateFile(const QString& filePath, const QString& displayName,
                                   const std::function<bool(QJsonDocument*, QString*)>& migrateBody,
                                   MigrationResult* result)
{
    if (!QFile::exists(filePath)) {
        return true;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (result) {
            result->ok = false;
            result->errors.append(file.errorString());
        }
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (result) {
            result->ok = false;
            result->errors.append(QStringLiteral("%1: %2").arg(displayName, parseError.errorString()));
        }
        return false;
    }

    const int schemaVersion = readSchemaVersion(document);
    if (schemaVersion >= currentSchemaVersion) {
        return true;
    }

    QString backupError;
    if (!SafeJsonWriter::createBackup(filePath, &backupError)) {
        if (result) {
            result->ok = false;
            result->errors.append(backupError);
        }
        return false;
    }
    if (result) {
        result->backups.append(filePath + QStringLiteral(".bak"));
        result->logLines.append(QStringLiteral("Backup created: %1.bak").arg(displayName));
        result->logLines.append(QStringLiteral("Migration started: %1 schema %2 → %3")
                                   .arg(displayName)
                                   .arg(schemaVersion)
                                   .arg(currentSchemaVersion));
    }

    QString migrateError;
    if (!migrateBody(&document, &migrateError)) {
        if (result) {
            result->ok = false;
            result->errors.append(migrateError);
        }
        return false;
    }

    if (!SafeJsonWriter::writeDocument(filePath, document, &migrateError)) {
        if (result) {
            result->ok = false;
            result->errors.append(migrateError);
        }
        return false;
    }

    if (result) {
        result->migratedFiles.append(filePath);
        result->logLines.append(QStringLiteral("Migration completed: %1").arg(displayName));
    }
    return true;
}

} // namespace zarya
