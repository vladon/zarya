#pragma once

#include "backup/BackupCategory.h"

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QStringList>

namespace zarya {

struct BackupCategoryEntry {
    bool included = false;
    QString file;
    int count = -1;
    bool redacted = false;
};

struct BackupManifest {
    static constexpr int currentFormatVersion = 1;

    QString format = QStringLiteral("zarya-backup");
    int formatVersion = currentFormatVersion;
    QString appName = QStringLiteral("Zarya");
    QString appVersion;
    QDateTime createdAt;
    QString createdBy = QStringLiteral("Zarya");
    QString platform;
    bool portableMode = false;
    bool redacted = false;
    QString redactionMode;
    QHash<QString, BackupCategoryEntry> categories;
    QStringList warnings;
    QHash<QString, QString> checksums;
};

class BackupManifestIO {
public:
    static bool write(const BackupManifest& manifest, const QString& filePath, QString* error);
    static bool read(const QString& filePath, BackupManifest* manifest, QString* error);
    static int countItemsInJsonFile(const QString& filePath);
};

} // namespace zarya
