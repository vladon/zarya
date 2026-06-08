#include "migration/MigrationManager.h"

#include "migration/JsonFileMigrator.h"
#include "storage/AppPaths.h"

#include <QJsonArray>
#include <QJsonObject>

namespace zarya {

namespace {

MigrationResult g_lastResult;

} // namespace

namespace {

bool migrateObjectFile(QJsonDocument* document, const QString& itemsKey, QString* error)
{
    Q_UNUSED(error);
    if (document->isArray()) {
        QJsonObject root;
        root.insert(itemsKey, document->array());
        JsonFileMigrator::wrapWithSchema(&root);
        *document = QJsonDocument(root);
        return true;
    }
    if (!document->isObject()) {
        return false;
    }
    QJsonObject root = document->object();
    JsonFileMigrator::wrapWithSchema(&root);
    *document = QJsonDocument(root);
    return true;
}

} // namespace

MigrationResult MigrationManager::runStartupMigrations()
{
    MigrationResult result;

    const auto migrate = [&](const QString& path, const QString& name, const QString& itemsKey) {
        return JsonFileMigrator::migrateFile(
            path, name,
            [&](QJsonDocument* document, QString* error) {
                return migrateObjectFile(document, itemsKey, error);
            },
            &result);
    };

    migrate(AppPaths::profilesFilePath(), QStringLiteral("profiles.json"),
            QStringLiteral("profiles"));
    migrate(AppPaths::subscriptionsFilePath(), QStringLiteral("subscriptions.json"),
            QStringLiteral("subscriptions"));
    migrate(AppPaths::routingFilePath(), QStringLiteral("routing.json"), QStringLiteral("profiles"));
    migrate(AppPaths::dnsFilePath(), QStringLiteral("dns.json"), QStringLiteral("profiles"));

    JsonFileMigrator::migrateFile(
        AppPaths::ruleSetStorePath(), QStringLiteral("rulesets.json"),
        [](QJsonDocument* document, QString* error) {
            Q_UNUSED(error);
            if (document->isArray()) {
                QJsonObject root;
                root.insert(QStringLiteral("custom"), document->array());
                JsonFileMigrator::wrapWithSchema(&root);
                *document = QJsonDocument(root);
                return true;
            }
            if (document->isObject()) {
                QJsonObject root = document->object();
                JsonFileMigrator::wrapWithSchema(&root);
                *document = QJsonDocument(root);
                return true;
            }
            return false;
        },
        &result);

    g_lastResult = result;
    return result;
}

const MigrationResult& MigrationManager::lastResult()
{
    return g_lastResult;
}

} // namespace zarya
