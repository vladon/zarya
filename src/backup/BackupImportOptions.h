#pragma once

#include "backup/BackupCategory.h"

#include <QHash>
#include <QString>

namespace zarya {

enum class ImportMode {
    Replace,
    Merge,
    Skip,
};

struct BackupImportOptions {
    QHash<BackupCategory, ImportMode> categoryModes;
    bool importMachineSpecificSettings = false;
    QString archivePath;
    QString stagingDir;
};

} // namespace zarya
