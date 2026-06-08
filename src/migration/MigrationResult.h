#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

struct MigrationResult {
    bool ok = true;
    QStringList migratedFiles;
    QStringList backups;
    QStringList errors;
    QStringList logLines;
};

} // namespace zarya
