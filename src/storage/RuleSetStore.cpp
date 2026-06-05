#include "storage/RuleSetStore.h"

#include "rulesets/RuleSetKind.h"
#include "storage/AppPaths.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

RuleSetItem itemFromJson(const QJsonObject& object)
{
    RuleSetItem item;
    item.id = object.value(QStringLiteral("id")).toString();
    item.tag = object.value(QStringLiteral("tag")).toString(item.id);
    item.name = object.value(QStringLiteral("name")).toString(item.tag);
    item.kind = ruleSetKindFromString(object.value(QStringLiteral("kind")).toString());
    item.preferredFormat =
        ruleSetFormatFromString(object.value(QStringLiteral("preferredFormat")).toString());
    item.srsUrl = QUrl(object.value(QStringLiteral("srsUrl")).toString());
    item.jsonUrl = QUrl(object.value(QStringLiteral("jsonUrl")).toString());
    item.checksumUrl = QUrl(object.value(QStringLiteral("checksumUrl")).toString());
    item.expectedSha256 = object.value(QStringLiteral("expectedSha256")).toString();
    item.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    item.description = object.value(QStringLiteral("description")).toString();
    item.builtIn = false;
    return item;
}

QJsonObject itemToJson(const RuleSetItem& item)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), item.id);
    object.insert(QStringLiteral("tag"), item.tag);
    object.insert(QStringLiteral("name"), item.name);
    object.insert(QStringLiteral("kind"), ruleSetKindToString(item.kind));
    object.insert(QStringLiteral("preferredFormat"), ruleSetFormatToString(item.preferredFormat));
    if (item.srsUrl.isValid()) {
        object.insert(QStringLiteral("srsUrl"), item.srsUrl.toString());
    }
    if (item.jsonUrl.isValid()) {
        object.insert(QStringLiteral("jsonUrl"), item.jsonUrl.toString());
    }
    if (item.checksumUrl.isValid()) {
        object.insert(QStringLiteral("checksumUrl"), item.checksumUrl.toString());
    }
    if (!item.expectedSha256.isEmpty()) {
        object.insert(QStringLiteral("expectedSha256"), item.expectedSha256);
    }
    object.insert(QStringLiteral("enabled"), item.enabled);
    object.insert(QStringLiteral("description"), item.description);
    return object;
}

} // namespace

bool RuleSetStore::load(QVector<RuleSetItem>* customItems, QString* errorMessage) const
{
    if (!customItems) {
        return false;
    }
    customItems->clear();
    const QString path = AppPaths::ruleSetStorePath();
    if (!QFile::exists(path)) {
        return true;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue& value : array) {
        customItems->append(itemFromJson(value.toObject()));
    }
    return true;
}

bool RuleSetStore::save(const QVector<RuleSetItem>& customItems, QString* errorMessage) const
{
    QJsonArray array;
    for (const RuleSetItem& item : customItems) {
        if (item.builtIn) {
            continue;
        }
        array.append(itemToJson(item));
    }
    QFile file(AppPaths::ruleSetStorePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace zarya
