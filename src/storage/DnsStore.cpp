#include "storage/DnsStore.h"

#include "domain/DnsProfileMode.h"
#include "domain/DnsQueryStrategy.h"
#include "domain/DnsServer.h"
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

QJsonObject serverToJson(const DnsServer& server)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), server.id);
    object.insert(QStringLiteral("enabled"), server.enabled);
    object.insert(QStringLiteral("kind"), dnsServerKindToString(server.kind));
    object.insert(QStringLiteral("address"), server.address);
    object.insert(QStringLiteral("port"), server.port);
    object.insert(QStringLiteral("queryStrategy"), server.queryStrategy);
    object.insert(QStringLiteral("timeoutMs"), server.timeoutMs);
    object.insert(QStringLiteral("tag"), server.tag);
    object.insert(QStringLiteral("skipFallback"), server.skipFallback);
    object.insert(QStringLiteral("note"), server.note);

    QJsonArray domains;
    for (const QString& domain : server.domains) {
        domains.append(domain);
    }
    object.insert(QStringLiteral("domains"), domains);

    QJsonArray expectIPs;
    for (const QString& expectIp : server.expectIPs) {
        expectIPs.append(expectIp);
    }
    object.insert(QStringLiteral("expectIPs"), expectIPs);
    return object;
}

DnsServer serverFromJson(const QJsonObject& object)
{
    DnsServer server;
    server.id = object.value(QStringLiteral("id")).toString();
    server.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    server.kind = dnsServerKindFromString(object.value(QStringLiteral("kind")).toString());
    server.address = object.value(QStringLiteral("address")).toString();
    server.port = object.value(QStringLiteral("port")).toInt(0);
    server.queryStrategy = object.value(QStringLiteral("queryStrategy")).toString();
    server.timeoutMs = object.value(QStringLiteral("timeoutMs")).toInt(0);
    server.tag = object.value(QStringLiteral("tag")).toString();
    server.skipFallback = object.value(QStringLiteral("skipFallback")).toBool(false);
    server.note = object.value(QStringLiteral("note")).toString();

    const QJsonArray domains = object.value(QStringLiteral("domains")).toArray();
    for (const QJsonValue& value : domains) {
        server.domains.append(value.toString());
    }
    const QJsonArray expectIPs = object.value(QStringLiteral("expectIPs")).toArray();
    for (const QJsonValue& value : expectIPs) {
        server.expectIPs.append(value.toString());
    }
    return server;
}

QJsonObject profileToJson(const DnsProfile& profile)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), profile.id);
    object.insert(QStringLiteral("name"), profile.name);
    object.insert(QStringLiteral("mode"), dnsProfileModeToString(profile.mode));
    object.insert(QStringLiteral("enabled"), profile.enabled);
    object.insert(QStringLiteral("isBuiltIn"), profile.isBuiltIn);
    object.insert(QStringLiteral("queryStrategy"), dnsQueryStrategyToString(profile.queryStrategy));
    object.insert(QStringLiteral("disableCache"), profile.disableCache);
    object.insert(QStringLiteral("disableFallback"), profile.disableFallback);
    object.insert(QStringLiteral("disableFallbackIfMatch"), profile.disableFallbackIfMatch);
    if (profile.createdAt.isValid()) {
        object.insert(QStringLiteral("createdAt"), profile.createdAt.toString(Qt::ISODate));
    }
    if (profile.updatedAt.isValid()) {
        object.insert(QStringLiteral("updatedAt"), profile.updatedAt.toString(Qt::ISODate));
    }

    QJsonObject hosts;
    for (auto it = profile.hosts.constBegin(); it != profile.hosts.constEnd(); ++it) {
        hosts.insert(it.key(), it.value());
    }
    object.insert(QStringLiteral("hosts"), hosts);

    QJsonArray servers;
    for (const DnsServer& server : profile.servers) {
        servers.append(serverToJson(server));
    }
    object.insert(QStringLiteral("servers"), servers);
    return object;
}

DnsProfile profileFromJson(const QJsonObject& object)
{
    DnsProfile profile;
    profile.id = object.value(QStringLiteral("id")).toString();
    profile.name = object.value(QStringLiteral("name")).toString();
    profile.mode = dnsProfileModeFromString(object.value(QStringLiteral("mode")).toString());
    profile.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    profile.isBuiltIn = object.value(QStringLiteral("isBuiltIn")).toBool(false);
    profile.queryStrategy =
        dnsQueryStrategyFromString(object.value(QStringLiteral("queryStrategy")).toString());
    profile.disableCache = object.value(QStringLiteral("disableCache")).toBool(false);
    profile.disableFallback = object.value(QStringLiteral("disableFallback")).toBool(false);
    profile.disableFallbackIfMatch =
        object.value(QStringLiteral("disableFallbackIfMatch")).toBool(false);
    profile.createdAt =
        QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    profile.updatedAt =
        QDateTime::fromString(object.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);

    const QJsonObject hosts = object.value(QStringLiteral("hosts")).toObject();
    for (auto it = hosts.constBegin(); it != hosts.constEnd(); ++it) {
        profile.hosts.insert(it.key(), it.value().toString());
    }

    const QJsonArray servers = object.value(QStringLiteral("servers")).toArray();
    for (const QJsonValue& value : servers) {
        profile.servers.append(serverFromJson(value.toObject()));
    }
    return profile;
}

} // namespace

DnsStore::DnsStore(QString filePath)
    : m_filePath(filePath.isEmpty() ? AppPaths::dnsFilePath() : std::move(filePath))
{
}

QString DnsStore::filePath() const
{
    return m_filePath;
}

void DnsStore::setFilePath(const QString& path)
{
    m_filePath = path;
}

QVector<DnsProfile> DnsStore::load(QString* errorMessage) const
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

    QVector<DnsProfile> profiles;
    const QJsonArray array = document.object().value(QStringLiteral("profiles")).toArray();
    for (const QJsonValue& value : array) {
        profiles.append(profileFromJson(value.toObject()));
    }
    return profiles;
}

bool DnsStore::save(const QVector<DnsProfile>& profiles, QString* errorMessage) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonArray array;
    for (const DnsProfile& profile : profiles) {
        array.append(profileToJson(profile));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("profiles"), array);
    JsonFileMigrator::wrapWithSchema(&root);

    return SafeJsonWriter::writeDocument(m_filePath, QJsonDocument(root), errorMessage);
}

} // namespace zarya
