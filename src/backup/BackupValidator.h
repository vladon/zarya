#pragma once

#include "backup/BackupManifest.h"

#include <QString>
#include <QStringList>

namespace zarya {

struct BackupValidationResult {
    bool valid = false;
    bool canImport = false;
    QStringList errors;
    QStringList warnings;
};

class BackupValidator {
public:
    static BackupValidationResult validateManifest(const BackupManifest& manifest);
    static bool verifyChecksums(const BackupManifest& manifest, const QString& stagingDir,
                                QStringList* errors);
    static QString sha256OfFile(const QString& filePath);
};

} // namespace zarya
