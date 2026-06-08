#pragma once

#include "backup/BackupExportOptions.h"

#include <QString>
#include <QStringList>

namespace zarya {

struct RedactionReport {
    BackupRedactionMode mode = BackupRedactionMode::Strict;
    QStringList redactedFields;
};

class BackupRedactor {
public:
    static bool redactFile(const QString& filePath, BackupRedactionMode mode,
                           RedactionReport* report, QString* error);
    static bool writeRedactionReport(const QString& filePath, const RedactionReport& report,
                                     QString* error);
};

} // namespace zarya
