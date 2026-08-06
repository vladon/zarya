#pragma once

#include "runtime/core/ICoreRuntimeHost.h"

#include <functional>

namespace zarya {

class CoreManager;

class ProcessCoreRuntimeHost final : public ICoreRuntimeHost {
    Q_OBJECT

public:
    using ExecutableResolver = std::function<QString(CoreType)>;

    ProcessCoreRuntimeHost(CoreManager* manager, CoreDistributionKind distributionKind,
                           ExecutableResolver executableResolver, QObject* parent = nullptr);

    CoreDistributionKind distributionKind() const override;
    CoreRuntimeCapabilities capabilities() const override;
    bool isAvailable() const override;
    QString version() const override;
    int abiVersion() const override;
    QString loadStatus() const override;
    CoreRuntimeState state() const override;

    CoreOperationResult validate(const CoreLaunchRequest& request) override;
    CoreOperationResult start(const CoreLaunchRequest& request) override;
    CoreOperationResult stop() override;

private:
    CoreOperationResult materializeConfig(const CoreLaunchRequest& request, QString* path) const;
    QString executablePath(CoreType type) const;

    CoreManager* m_manager = nullptr;
    CoreDistributionKind m_distributionKind = CoreDistributionKind::ExternalExecutable;
    ExecutableResolver m_executableResolver;
    CoreRuntimeState m_state = CoreRuntimeState::Stopped;
    QString m_activeConfigPath;
};

} // namespace zarya
