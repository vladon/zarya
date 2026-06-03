#include "storage/SubscriptionStore.h"

#include "storage/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace zarya {

namespace {

QJsonObject subscriptionToJson(const Subscription& subscription)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), subscription.id);
    object.insert(QStringLiteral("name"), subscription.name);
    object.insert(QStringLiteral("url"), subscription.url);
    object.insert(QStringLiteral("enabled"), subscription.enabled);
    if (subscription.lastUpdatedAt.isValid()) {
        object.insert(QStringLiteral("lastUpdatedAt"), subscription.lastUpdatedAt.toString(Qt::ISODate));
    }
    object.insert(QStringLiteral("lastStatus"), subscriptionStatusToString(subscription.lastStatus));
    object.insert(QStringLiteral("lastError"), subscription.lastError);
    object.insert(QStringLiteral("profileCount"), subscription.profileCount);
    if (!subscription.userAgent.isEmpty()) {
        object.insert(QStringLiteral("userAgent"), subscription.userAgent);
    }
    if (!subscription.remarks.isEmpty()) {
        object.insert(QStringLiteral("remarks"), subscription.remarks);
    }
    return object;
}

Subscription subscriptionFromJson(const QJsonObject& object)
{
    Subscription subscription;
    subscription.id = object.value(QStringLiteral("id")).toString();
    subscription.name = object.value(QStringLiteral("name")).toString();
    subscription.url = object.value(QStringLiteral("url")).toString();
    subscription.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    subscription.lastUpdatedAt =
        QDateTime::fromString(object.value(QStringLiteral("lastUpdatedAt")).toString(), Qt::ISODate);
    subscription.lastStatus =
        subscriptionStatusFromString(object.value(QStringLiteral("lastStatus")).toString());
    subscription.lastError = object.value(QStringLiteral("lastError")).toString();
    subscription.profileCount = object.value(QStringLiteral("profileCount")).toInt(0);
    subscription.userAgent = object.value(QStringLiteral("userAgent")).toString();
    subscription.remarks = object.value(QStringLiteral("remarks")).toString();

    if (subscription.id.isEmpty()) {
        subscription.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (!subscription.enabled) {
        subscription.lastStatus = SubscriptionStatus::Disabled;
    }
    return subscription;
}

} // namespace

SubscriptionStore::SubscriptionStore(QString filePath)
    : m_filePath(filePath.isEmpty() ? AppPaths::subscriptionsFilePath() : std::move(filePath))
{
}

QString SubscriptionStore::filePath() const
{
    return m_filePath;
}

void SubscriptionStore::setFilePath(const QString& path)
{
    m_filePath = path;
}

QVector<Subscription> SubscriptionStore::load(QString* errorMessage) const
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

    const QJsonArray array = document.object().value(QStringLiteral("subscriptions")).toArray();
    QVector<Subscription> subscriptions;
    subscriptions.reserve(array.size());
    for (const QJsonValue& value : array) {
        subscriptions.append(subscriptionFromJson(value.toObject()));
    }
    return subscriptions;
}

bool SubscriptionStore::save(const QVector<Subscription>& subscriptions, QString* errorMessage) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonArray array;
    for (const Subscription& subscription : subscriptions) {
        array.append(subscriptionToJson(subscription));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("subscriptions"), array);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace zarya
