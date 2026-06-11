#pragma once

#include "updater/runner/UpdatePlanFile.h"
#include "updater/runner/UpdaterLog.h"

namespace zarya {

class UpdateRollback {
public:
    explicit UpdateRollback(UpdaterLog& log);

    bool rollback(const UpdatePlan& plan, QString* errorMessage = nullptr);
    bool restartPreviousVersion(const UpdatePlan& plan, const QStringList& extraArgs);

private:
    static bool copyTree(const QString& sourceRoot, const QString& destinationRoot,
                         const QStringList& preservePaths, UpdaterLog* log);

    UpdaterLog& m_log;
};

} // namespace zarya
