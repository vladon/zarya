#include "killswitch/KillSwitchMarker.h"

#include "storage/AppPaths.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

QString KillSwitchMarker::markerPath()
{
    return AppPaths::killSwitchMarkerPath();
}

bool KillSwitchMarker::exists()
{
    return QFile::exists(markerPath());
}

bool KillSwitchMarker::read(KillSwitchMarkerData* data, QString* errorMessage)
{
    if (!data) {
        return false;
    }
    QFile file(markerPath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    data->version = object.value(QStringLiteral("version")).toInt(1);
    data->backend = object.value(QStringLiteral("backend")).toString();
    data->enabledAt = object.value(QStringLiteral("enabledAt")).toString();
    data->rulesetName = object.value(QStringLiteral("rulesetName")).toString();
    data->tunInterfaceName = object.value(QStringLiteral("tunInterfaceName")).toString();
    return true;
}

bool KillSwitchMarker::write(const KillSwitchMarkerData& data, QString* errorMessage)
{
    AppPaths::ensureKillSwitchDir();
    QJsonObject object;
    object.insert(QStringLiteral("version"), data.version);
    object.insert(QStringLiteral("backend"), data.backend);
    object.insert(QStringLiteral("enabledAt"), data.enabledAt);
    object.insert(QStringLiteral("rulesetName"), data.rulesetName);
    object.insert(QStringLiteral("tunInterfaceName"), data.tunInterfaceName);

    QFile file(markerPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

bool KillSwitchMarker::remove(QString* errorMessage)
{
    if (!QFile::exists(markerPath())) {
        return true;
    }
    if (!QFile::remove(markerPath())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not remove kill switch marker.");
        }
        return false;
    }
    return true;
}

} // namespace zarya
