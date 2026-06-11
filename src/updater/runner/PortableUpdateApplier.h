#pragma once

#include "updater/runner/UpdatePlanFile.h"
#include "updater/runner/UpdaterLog.h"

namespace zarya {

class PortableUpdateApplier {
public:
    explicit PortableUpdateApplier(UpdaterLog& log);

    bool apply(const UpdatePlan& plan, QString* errorMessage = nullptr);

private:
    bool waitForMainProcessExit(const UpdatePlan& plan, int timeoutMs, QString* errorMessage);
    bool createBackup(const UpdatePlan& plan, QString* errorMessage);
    bool removeAppFilesExceptPreserved(const UpdatePlan& plan, QString* errorMessage);
    bool copyStagedFiles(const UpdatePlan& plan, QString* errorMessage);
    bool runPostUpdateCheck(const UpdatePlan& plan, QString* errorMessage);
    bool restartApplication(const UpdatePlan& plan, QString* errorMessage);
    void writeStateFile(const UpdatePlan& plan, const QString& fileName,
                        const QJsonObject& extraFields);
    void pruneOldBackups(const UpdatePlan& plan, int retentionCount);

    static bool isPreservedRelativePath(const QString& relativePath,
                                        const QStringList& preservePaths);
    static bool copyPathRecursively(const QString& sourcePath, const QString& destinationPath,
                                    const QStringList& skipRelativePrefixes,
                                    const QString& sourceRoot, UpdaterLog* log);
    static bool removePathRecursively(const QString& path);

    UpdaterLog& m_log;
};

} // namespace zarya
