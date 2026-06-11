#include "updater/AppUpdateAsset.h"

namespace zarya {

bool AppUpdateAsset::isValid() const
{
    return !platform.isEmpty() && !fileName.isEmpty() && !url.isEmpty();
}

AppUpdateAsset AppUpdateAsset::fromJson(const QJsonObject& object)
{
    AppUpdateAsset asset;
    asset.platform = object.value(QStringLiteral("platform")).toString().trimmed().toLower();
    asset.architecture =
        object.value(QStringLiteral("architecture")).toString().trimmed().toLower();
    asset.installationMode =
        object.value(QStringLiteral("installationMode")).toString().trimmed().toLower();
    asset.fileName = object.value(QStringLiteral("fileName")).toString();
    asset.url = object.value(QStringLiteral("url")).toString();
    asset.sizeBytes = static_cast<qint64>(object.value(QStringLiteral("sizeBytes")).toDouble());
    asset.sha256 = object.value(QStringLiteral("sha256")).toString().trimmed().toLower();

    const QJsonObject signatureObject = object.value(QStringLiteral("signature")).toObject();
    asset.signature.type = signatureObject.value(QStringLiteral("type")).toString();
    if (!signatureObject.value(QStringLiteral("url")).isNull()) {
        asset.signature.url = signatureObject.value(QStringLiteral("url")).toString();
    }
    return asset;
}

QJsonObject AppUpdateAsset::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("platform"), platform);
    object.insert(QStringLiteral("architecture"), architecture);
    object.insert(QStringLiteral("installationMode"), installationMode);
    object.insert(QStringLiteral("fileName"), fileName);
    object.insert(QStringLiteral("url"), url);
    object.insert(QStringLiteral("sizeBytes"), static_cast<double>(sizeBytes));
    object.insert(QStringLiteral("sha256"), sha256);
    QJsonObject signatureObject;
    signatureObject.insert(QStringLiteral("type"), signature.type);
    if (signature.url.isEmpty()) {
        signatureObject.insert(QStringLiteral("url"), QJsonValue::Null);
    } else {
        signatureObject.insert(QStringLiteral("url"), signature.url);
    }
    object.insert(QStringLiteral("signature"), signatureObject);
    return object;
}

} // namespace zarya
