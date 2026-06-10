#include "platform/SystemProxyStateStore.h"

#include "storage/AppPaths.h"
#include "storage/SafeJsonWriter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

QString storePath()
{
    return AppPaths::dataDir() + QStringLiteral("/proxy-previous-state.json");
}

QJsonObject toJson(const SystemProxyState& state)
{
    QJsonObject object;
    object.insert(QStringLiteral("proxyEnabled"), state.proxyEnabled);
    object.insert(QStringLiteral("proxyServer"), state.proxyServer);
    object.insert(QStringLiteral("proxyOverride"), state.proxyOverride);
    object.insert(QStringLiteral("autoDetect"), state.autoDetect);
    object.insert(QStringLiteral("autoConfigUrl"), state.autoConfigUrl);
    object.insert(QStringLiteral("platform"), state.platform);
    object.insert(QStringLiteral("backend"), state.backend);
    object.insert(QStringLiteral("rawValues"), QJsonObject::fromVariantMap(state.rawValues));
    object.insert(QStringLiteral("activeNetworkService"), state.activeNetworkService);
    object.insert(QStringLiteral("supportLevel"), state.supportLevel);

    QJsonArray services;
    for (const QString& service : state.affectedNetworkServices) {
        services.append(service);
    }
    object.insert(QStringLiteral("affectedNetworkServices"), services);
    return object;
}

bool fromJson(const QJsonObject& object, SystemProxyState* state)
{
    if (!state) {
        return false;
    }
    state->proxyEnabled = object.value(QStringLiteral("proxyEnabled")).toBool();
    state->proxyServer = object.value(QStringLiteral("proxyServer")).toString();
    state->proxyOverride = object.value(QStringLiteral("proxyOverride")).toString();
    state->autoDetect = object.value(QStringLiteral("autoDetect")).toBool();
    state->autoConfigUrl = object.value(QStringLiteral("autoConfigUrl")).toString();
    state->platform = object.value(QStringLiteral("platform")).toString();
    state->backend = object.value(QStringLiteral("backend")).toString();
    state->rawValues = object.value(QStringLiteral("rawValues")).toObject().toVariantMap();
    state->activeNetworkService = object.value(QStringLiteral("activeNetworkService")).toString();
    state->supportLevel = object.value(QStringLiteral("supportLevel")).toString();
    state->affectedNetworkServices.clear();
    const QJsonArray services = object.value(QStringLiteral("affectedNetworkServices")).toArray();
    for (const QJsonValue& value : services) {
        state->affectedNetworkServices.append(value.toString());
    }
    return true;
}

} // namespace

bool SystemProxyStateStore::save(const SystemProxyState& state)
{
    const QJsonDocument document(toJson(state));
    return SafeJsonWriter::writeDocument(storePath(), document, nullptr);
}

bool SystemProxyStateStore::load(SystemProxyState* state)
{
    if (!state) {
        return false;
    }
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }
    return fromJson(document.object(), state);
}

void SystemProxyStateStore::clear()
{
    QFile::remove(storePath());
}

bool SystemProxyStateStore::exists()
{
    return QFile::exists(storePath());
}

} // namespace zarya
