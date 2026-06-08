#include "storage/RoutingStore.h"

#include "domain/RoutingMode.h"
#include "migration/JsonFileMigrator.h"
#include "storage/AppPaths.h"
#include "storage/SafeJsonWriter.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

QJsonObject ruleToJson(const RoutingRule& rule)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), rule.id);
    object.insert(QStringLiteral("enabled"), rule.enabled);
    object.insert(QStringLiteral("type"), routingRuleTypeToString(rule.type));
    object.insert(QStringLiteral("action"), routingActionToString(rule.action));
    object.insert(QStringLiteral("note"), rule.note);

    QJsonArray values;
    for (const QString& value : rule.values) {
        values.append(value);
    }
    object.insert(QStringLiteral("values"), values);
    return object;
}

RoutingRule ruleFromJson(const QJsonObject& object)
{
    RoutingRule rule;
    rule.id = object.value(QStringLiteral("id")).toString();
    rule.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    rule.type = routingRuleTypeFromString(object.value(QStringLiteral("type")).toString());
    rule.action = routingActionFromString(object.value(QStringLiteral("action")).toString());
    rule.note = object.value(QStringLiteral("note")).toString();
    const QJsonArray values = object.value(QStringLiteral("values")).toArray();
    for (const QJsonValue& value : values) {
        rule.values.append(value.toString());
    }
    return rule;
}

QJsonObject profileToJson(const RoutingProfile& profile)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), profile.id);
    object.insert(QStringLiteral("name"), profile.name);
    object.insert(QStringLiteral("mode"), routingModeToString(profile.mode));
    object.insert(QStringLiteral("enabled"), profile.enabled);
    object.insert(QStringLiteral("domainStrategy"), profile.domainStrategy);
    object.insert(QStringLiteral("isBuiltIn"), profile.isBuiltIn);
    if (profile.createdAt.isValid()) {
        object.insert(QStringLiteral("createdAt"), profile.createdAt.toString(Qt::ISODate));
    }
    if (profile.updatedAt.isValid()) {
        object.insert(QStringLiteral("updatedAt"), profile.updatedAt.toString(Qt::ISODate));
    }

    QJsonArray rules;
    for (const RoutingRule& rule : profile.rules) {
        rules.append(ruleToJson(rule));
    }
    object.insert(QStringLiteral("rules"), rules);
    return object;
}

RoutingProfile profileFromJson(const QJsonObject& object)
{
    RoutingProfile profile;
    profile.id = object.value(QStringLiteral("id")).toString();
    profile.name = object.value(QStringLiteral("name")).toString();
    profile.mode = routingModeFromString(object.value(QStringLiteral("mode")).toString());
    profile.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    profile.domainStrategy = object.value(QStringLiteral("domainStrategy")).toString();
    if (profile.domainStrategy.isEmpty()) {
        profile.domainStrategy = QStringLiteral("AsIs");
    }
    profile.isBuiltIn = object.value(QStringLiteral("isBuiltIn")).toBool(false);
    profile.createdAt =
        QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    profile.updatedAt =
        QDateTime::fromString(object.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);

    const QJsonArray rules = object.value(QStringLiteral("rules")).toArray();
    for (const QJsonValue& value : rules) {
        profile.rules.append(ruleFromJson(value.toObject()));
    }
    return profile;
}

} // namespace

RoutingStore::RoutingStore(QString filePath)
    : m_filePath(filePath.isEmpty() ? AppPaths::routingFilePath() : std::move(filePath))
{
}

QString RoutingStore::filePath() const
{
    return m_filePath;
}

void RoutingStore::setFilePath(const QString& path)
{
    m_filePath = path;
}

QVector<RoutingProfile> RoutingStore::load(QString* errorMessage) const
{
    QFile file(m_filePath);
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return {};
    }

    QVector<RoutingProfile> profiles;
    const QJsonArray array = document.object().value(QStringLiteral("profiles")).toArray();
    profiles.reserve(array.size());
    for (const QJsonValue& value : array) {
        profiles.append(profileFromJson(value.toObject()));
    }
    return profiles;
}

bool RoutingStore::save(const QVector<RoutingProfile>& profiles, QString* errorMessage) const
{
    QJsonArray array;
    for (const RoutingProfile& profile : profiles) {
        array.append(profileToJson(profile));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("profiles"), array);
    JsonFileMigrator::wrapWithSchema(&root);

    return SafeJsonWriter::writeDocument(m_filePath, QJsonDocument(root), errorMessage);
}

} // namespace zarya
