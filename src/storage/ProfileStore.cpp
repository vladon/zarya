#include "storage/ProfileStore.h"

#include "storage/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

void insertIfNotEmpty(QJsonObject& object, const QString& key, const QString& value)
{
    if (!value.isEmpty()) {
        object.insert(key, value);
    }
}

QJsonObject profileToJson(const Profile& profile)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), profile.id);
    object.insert(QStringLiteral("name"), profile.name);
    object.insert(QStringLiteral("protocol"), protocolTypeToString(profile.protocol));
    object.insert(QStringLiteral("coreType"), coreTypeToString(profile.coreType));
    object.insert(QStringLiteral("address"), profile.address);
    object.insert(QStringLiteral("port"), profile.port);
    object.insert(QStringLiteral("uuid"), profile.uuidPassword);
    object.insert(QStringLiteral("uuidPassword"), profile.uuidPassword);
    object.insert(QStringLiteral("encryption"), profile.encryption);
    object.insert(QStringLiteral("flow"), profile.flow);
    object.insert(QStringLiteral("remark"), profile.remark);
    object.insert(QStringLiteral("enabled"), profile.enabled);

    object.insert(QStringLiteral("network"), profile.network);
    insertIfNotEmpty(object, QStringLiteral("path"), profile.path);
    insertIfNotEmpty(object, QStringLiteral("host"), profile.host);
    insertIfNotEmpty(object, QStringLiteral("headerType"), profile.headerType);

    object.insert(QStringLiteral("security"), profile.security);
    insertIfNotEmpty(object, QStringLiteral("serverName"), profile.serverName);
    insertIfNotEmpty(object, QStringLiteral("sni"), profile.sni);
    insertIfNotEmpty(object, QStringLiteral("publicKey"), profile.publicKey);
    insertIfNotEmpty(object, QStringLiteral("shortId"), profile.shortId);
    insertIfNotEmpty(object, QStringLiteral("spiderX"), profile.spiderX);
    insertIfNotEmpty(object, QStringLiteral("fingerprint"), profile.fingerprint);
    if (profile.allowInsecure) {
        object.insert(QStringLiteral("allowInsecure"), true);
    }

    return object;
}

Profile profileFromJson(const QJsonObject& object)
{
    Profile profile;
    profile.id = object.value(QStringLiteral("id")).toString();
    profile.name = object.value(QStringLiteral("name")).toString();
    profile.protocol = protocolTypeFromString(object.value(QStringLiteral("protocol")).toString());
    profile.coreType = coreTypeFromString(object.value(QStringLiteral("coreType")).toString());
    profile.address = object.value(QStringLiteral("address")).toString();
    profile.port = object.value(QStringLiteral("port")).toInt(443);

    profile.uuidPassword = object.value(QStringLiteral("uuid")).toString();
    if (profile.uuidPassword.isEmpty()) {
        profile.uuidPassword = object.value(QStringLiteral("uuidPassword")).toString();
    }

    profile.encryption = object.value(QStringLiteral("encryption")).toString();
    if (profile.encryption.isEmpty()) {
        profile.encryption = QStringLiteral("none");
    }

    profile.security = object.value(QStringLiteral("security")).toString();
    profile.network = object.value(QStringLiteral("network")).toString();
    if (profile.network.isEmpty()) {
        profile.network = QStringLiteral("tcp");
    }
    profile.path = object.value(QStringLiteral("path")).toString();
    profile.host = object.value(QStringLiteral("host")).toString();
    profile.headerType = object.value(QStringLiteral("headerType")).toString();

    profile.sni = object.value(QStringLiteral("sni")).toString();
    profile.flow = object.value(QStringLiteral("flow")).toString();
    profile.remark = object.value(QStringLiteral("remark")).toString();
    profile.enabled = object.value(QStringLiteral("enabled")).toBool(true);

    profile.serverName = object.value(QStringLiteral("serverName")).toString();
    profile.publicKey = object.value(QStringLiteral("publicKey")).toString();
    profile.shortId = object.value(QStringLiteral("shortId")).toString();
    profile.spiderX = object.value(QStringLiteral("spiderX")).toString();
    profile.fingerprint = object.value(QStringLiteral("fingerprint")).toString();
    profile.allowInsecure = object.value(QStringLiteral("allowInsecure")).toBool(false);

    if (profile.id.isEmpty()) {
        profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    return profile;
}

} // namespace

ProfileStore::ProfileStore(QString filePath)
    : m_filePath(filePath.isEmpty() ? AppPaths::profilesFilePath() : std::move(filePath))
{
}

QString ProfileStore::filePath() const
{
    return m_filePath;
}

void ProfileStore::setFilePath(const QString& path)
{
    m_filePath = path;
}

QVector<Profile> ProfileStore::load(QString* errorMessage) const
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

    const QJsonArray array = document.object().value(QStringLiteral("profiles")).toArray();
    QVector<Profile> profiles;
    profiles.reserve(array.size());
    for (const QJsonValue& value : array) {
        profiles.append(profileFromJson(value.toObject()));
    }
    return profiles;
}

bool ProfileStore::save(const QVector<Profile>& profiles, QString* errorMessage) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonArray array;
    for (const Profile& profile : profiles) {
        array.append(profileToJson(profile));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 2);
    root.insert(QStringLiteral("profiles"), array);

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
