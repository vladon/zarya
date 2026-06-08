#pragma once

#include <QString>

namespace zarya {

struct BackupArchiveResult {
    bool ok = false;
    QString error;
};

class BackupArchive {
public:
    static BackupArchiveResult createZip(const QString& sourceDir, const QString& outputZipPath);
    static BackupArchiveResult extractZip(const QString& zipPath, const QString& destinationDir);
    static bool validateZipEntries(const QString& zipPath, QString* errorMessage);
};

} // namespace zarya
