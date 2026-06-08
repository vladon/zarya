#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

struct ExtractResult {
    bool ok = false;
    QString extractedDir;
    QStringList files;
    QString error;
};

class CoreArchiveExtractor {
public:
    static ExtractResult extract(const QString& archivePath, const QString& destinationDir);
    static bool validateArchiveEntries(const QString& archivePath, QString* errorMessage);

private:
    static QStringList listArchiveEntries(const QString& archivePath, QString* errorMessage);
    static bool hasUnsafePath(const QString& entry);
};

} // namespace zarya
