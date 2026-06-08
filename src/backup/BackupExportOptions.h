#pragma once

#include "backup/BackupCategory.h"

#include <QSet>
#include <QString>

namespace zarya {

enum class BackupRedactionMode {
    None,
    Basic,
    Strict,
};

struct BackupExportOptions {
    bool diagnosticBackup = false;
    BackupRedactionMode redactionMode = BackupRedactionMode::None;
    QSet<BackupCategory> categories;
    QString outputPath;
};

} // namespace zarya
