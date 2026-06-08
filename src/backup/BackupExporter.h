#pragma once

#include "backup/BackupExportOptions.h"
#include "backup/BackupManifest.h"
#include "backup/BackupRedactor.h"

#include <functional>
#include <QString>

namespace zarya {

class BackupExporter {
public:
    using LogCallback = std::function<void(const QString&)>;

    explicit BackupExporter(LogCallback log = {});

    bool exportToArchive(const BackupExportOptions& options, QString* error);

private:
    bool stageCategory(BackupCategory category, const QString& stagingDir,
                       BackupRedactionMode redactionMode, RedactionReport* report, QString* error);
    void addChecksums(const QString& stagingDir, BackupManifest* manifest);
    QString categoryArchivePath(BackupCategory category) const;

    LogCallback m_log;
};

} // namespace zarya
