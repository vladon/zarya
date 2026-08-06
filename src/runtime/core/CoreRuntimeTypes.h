#pragma once

#include "domain/CoreType.h"

#include <QByteArray>
#include <QFlags>
#include <QString>

namespace zarya {

struct CoreLaunchRequest {
    CoreType coreType = CoreType::Xray;
    QByteArray configJson;
    QString assetDir;
    QString dataDir;
};

struct CoreOperationResult {
    bool success = false;
    QString errorCode;
    QString message;

    static CoreOperationResult ok()
    {
        return {true, {}, {}};
    }

    static CoreOperationResult failure(const QString& code, const QString& text)
    {
        return {false, code, text};
    }
};

enum class CoreRuntimeState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Failed,
};

enum class CoreDistributionKind {
    Embedded,
    ManagedExecutable,
    ExternalExecutable,
};

enum class CoreRuntimeCapability : quint32 {
    None = 0,
    Validation = 1U << 0,
    Update = 1U << 1,
    Rollback = 1U << 2,
    HelperExecution = 1U << 3,
    ParallelInstances = 1U << 4,
};
Q_DECLARE_FLAGS(CoreRuntimeCapabilities, CoreRuntimeCapability)

} // namespace zarya

Q_DECLARE_OPERATORS_FOR_FLAGS(zarya::CoreRuntimeCapabilities)
