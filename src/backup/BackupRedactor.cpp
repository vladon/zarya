#include "backup/BackupRedactor.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

constexpr auto kRedacted = "<redacted>";
constexpr auto kRedactedHost = "<redacted-host>";
constexpr auto kRedactedUrl = "<redacted-url>";

void track(RedactionReport* report, const QString& field)
{
    if (report && !report->redactedFields.contains(field)) {
        report->redactedFields.append(field);
    }
}

void redactProfileObject(QJsonObject& object, BackupRedactionMode mode, RedactionReport* report)
{
    const auto redact = [&](const char* key, const QString& trackKey) {
        if (object.contains(QLatin1String(key))) {
            object.insert(QLatin1String(key), kRedacted);
            track(report, trackKey);
        }
    };

    redact("uuid", QStringLiteral("profiles.uuid"));
    redact("uuidPassword", QStringLiteral("profiles.uuidPassword"));
    redact("password", QStringLiteral("profiles.password"));
    redact("publicKey", QStringLiteral("profiles.publicKey"));
    redact("shortId", QStringLiteral("profiles.shortId"));

    if (mode == BackupRedactionMode::Strict) {
        if (object.contains(QStringLiteral("address"))) {
            object.insert(QStringLiteral("address"), kRedactedHost);
            track(report, QStringLiteral("profiles.address"));
        }
        if (object.contains(QStringLiteral("serverName"))) {
            object.insert(QStringLiteral("serverName"), kRedactedHost);
            track(report, QStringLiteral("profiles.serverName"));
        }
        if (object.contains(QStringLiteral("sni"))) {
            object.insert(QStringLiteral("sni"), kRedactedHost);
            track(report, QStringLiteral("profiles.sni"));
        }
        if (object.contains(QStringLiteral("host"))) {
            object.insert(QStringLiteral("host"), kRedactedHost);
            track(report, QStringLiteral("profiles.host"));
        }
    }
}

void redactSubscriptionObject(QJsonObject& object, BackupRedactionMode mode, RedactionReport* report)
{
    Q_UNUSED(mode);
    if (object.contains(QStringLiteral("url"))) {
        object.insert(QStringLiteral("url"), kRedactedUrl);
        track(report, QStringLiteral("subscriptions.url"));
    }
}

void redactSettingsValue(const QString& key, QJsonValue& value, BackupRedactionMode mode,
                         RedactionReport* report)
{
    if (key == QStringLiteral("cores/xrayPath") || key == QStringLiteral("cores/singBoxPath")) {
        const QString path = value.toString();
        if (!path.isEmpty()) {
            value = QFileInfo(path).fileName();
            track(report, QStringLiteral("settings.%1").arg(key));
        }
        return;
    }
    if (mode == BackupRedactionMode::Strict && key.startsWith(QStringLiteral("window/"))) {
        value = QJsonValue();
        track(report, QStringLiteral("settings.window"));
    }
}

} // namespace

bool BackupRedactor::redactFile(const QString& filePath, BackupRedactionMode mode,
                                RedactionReport* report, QString* error)
{
    if (mode == BackupRedactionMode::None) {
        return true;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return false;
    }

    if (report) {
        report->mode = mode;
    }

    QJsonDocument output;
    const QString baseName = QFileInfo(filePath).fileName();

    if (baseName == QStringLiteral("profiles.json") && document.isArray()) {
        QJsonArray array = document.array();
        for (int i = 0; i < array.size(); ++i) {
            QJsonObject object = array.at(i).toObject();
            redactProfileObject(object, mode, report);
            array.replace(i, object);
        }
        output = QJsonDocument(array);
    } else if (baseName == QStringLiteral("subscriptions.json") && document.isArray()) {
        QJsonArray array = document.array();
        for (int i = 0; i < array.size(); ++i) {
            QJsonObject object = array.at(i).toObject();
            redactSubscriptionObject(object, mode, report);
            array.replace(i, object);
        }
        output = QJsonDocument(array);
    } else if (baseName == QStringLiteral("settings.ini")) {
        return true;
    } else if (baseName == QStringLiteral("settings-export.json") && document.isObject()) {
        QJsonObject object = document.object();
        for (auto it = object.begin(); it != object.end(); ++it) {
            QJsonValue value = it.value();
            redactSettingsValue(it.key(), value, mode, report);
            object.insert(it.key(), value);
        }
        output = QJsonDocument(object);
    } else {
        return true;
    }

    file.close();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(output.toJson(QJsonDocument::Indented));
    return true;
}

bool BackupRedactor::writeRedactionReport(const QString& filePath, const RedactionReport& report,
                                          QString* error)
{
    QJsonObject root;
    root.insert(QStringLiteral("mode"),
                report.mode == BackupRedactionMode::Strict ? QStringLiteral("strict")
                                                           : QStringLiteral("basic"));
    QJsonArray fields;
    for (const QString& field : report.redactedFields) {
        fields.append(field);
    }
    root.insert(QStringLiteral("redactedFields"), fields);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace zarya
