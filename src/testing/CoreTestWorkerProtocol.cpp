#include "testing/CoreTestWorkerProtocol.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace zarya {

QJsonObject prepareIsolatedXrayTestConfig(QJsonObject config)
{
    config.insert(QStringLiteral("log"), QJsonObject{
        {QStringLiteral("loglevel"), QStringLiteral("none")},
    });
    return config;
}

bool parseCoreTestWorkerResponse(
    const QByteArray& output,
    QJsonObject* response,
    QString* errorMessage)
{
    const QList<QByteArray> lines = output.split('\n');
    for (auto i = lines.crbegin(); i != lines.crend(); ++i) {
        const QByteArray candidate = i->trimmed();
        if (candidate.isEmpty()) {
            continue;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(candidate, &parseError);
        if (document.isObject()
            && document.object().value(QStringLiteral("success")).isBool()) {
            if (response) {
                *response = document.object();
            }
            if (errorMessage) {
                errorMessage->clear();
            }
            return true;
        }
    }
    if (response) {
        *response = {};
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Worker did not return a valid JSON response object.");
    }
    return false;
}

} // namespace zarya
