#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace zarya {

struct DiagnosticsManifest {
    static constexpr int currentFormatVersion = 1;

    QString format = QStringLiteral("zarya-diagnostics");
    int formatVersion = currentFormatVersion;
    QString appName = QStringLiteral("Zarya");
    QString appVersion;
    QString createdAt;
    QString platform;
    bool portableMode = false;
    QString redactionMode = QStringLiteral("strict");
    bool secretsIncluded = false;
    QHash<QString, bool> categories;
    QStringList warnings;
    QHash<QString, QString> checksums;
};

class DiagnosticsManifestIO {
public:
    static bool write(const DiagnosticsManifest& manifest, const QString& filePath, QString* error);
};

} // namespace zarya
