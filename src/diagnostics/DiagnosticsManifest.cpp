#include "diagnostics/DiagnosticsManifest.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

bool DiagnosticsManifestIO::write(const DiagnosticsManifest& manifest, const QString& filePath,
                                  QString* error)
{
    QJsonObject root;
    root.insert(QStringLiteral("format"), manifest.format);
    root.insert(QStringLiteral("formatVersion"), manifest.formatVersion);
    root.insert(QStringLiteral("appName"), manifest.appName);
    root.insert(QStringLiteral("appVersion"), manifest.appVersion);
    root.insert(QStringLiteral("createdAt"), manifest.createdAt);
    root.insert(QStringLiteral("platform"), manifest.platform);
    root.insert(QStringLiteral("portableMode"), manifest.portableMode);

    QJsonObject redaction;
    redaction.insert(QStringLiteral("mode"), manifest.redactionMode);
    redaction.insert(QStringLiteral("secretsIncluded"), manifest.secretsIncluded);
    root.insert(QStringLiteral("redaction"), redaction);

    QJsonObject categories;
    for (auto it = manifest.categories.constBegin(); it != manifest.categories.constEnd(); ++it) {
        categories.insert(it.key(), it.value());
    }
    root.insert(QStringLiteral("categories"), categories);

    QJsonArray warnings;
    for (const QString& warning : manifest.warnings) {
        warnings.append(warning);
    }
    root.insert(QStringLiteral("warnings"), warnings);

    QJsonObject checksums;
    for (auto it = manifest.checksums.constBegin(); it != manifest.checksums.constEnd(); ++it) {
        checksums.insert(it.key(), it.value());
    }
    root.insert(QStringLiteral("checksums"), checksums);

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
