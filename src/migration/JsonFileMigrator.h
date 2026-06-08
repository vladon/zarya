#pragma once

#include "migration/MigrationResult.h"

#include <QJsonDocument>
#include <QString>
#include <functional>

namespace zarya {

class JsonFileMigrator {
public:
    static constexpr int currentSchemaVersion = 1;

    static bool migrateFile(const QString& filePath, const QString& displayName,
                            const std::function<bool(QJsonDocument*, QString*)>& migrateBody,
                            MigrationResult* result);
    static int readSchemaVersion(const QJsonDocument& document);
    static void wrapWithSchema(QJsonObject* object);
};

} // namespace zarya
