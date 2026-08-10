#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace zarya {

QJsonObject prepareIsolatedXrayTestConfig(QJsonObject config);

bool parseCoreTestWorkerResponse(
    const QByteArray& output,
    QJsonObject* response,
    QString* errorMessage);

} // namespace zarya
