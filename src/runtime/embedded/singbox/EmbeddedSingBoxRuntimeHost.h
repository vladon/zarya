#pragma once

#include "runtime/core/ICoreRuntimeHost.h"

#include <QLibrary>
#include <QTimer>

#include <cstddef>

namespace zarya {

class CoreRuntimeCoordinator;

class EmbeddedSingBoxRuntimeHost final : public ICoreRuntimeHost {
    Q_OBJECT

public:
    explicit EmbeddedSingBoxRuntimeHost(CoreRuntimeCoordinator* coordinator,
                                     QObject* parent = nullptr);

    CoreDistributionKind distributionKind() const override;
    CoreRuntimeCapabilities capabilities() const override;
    bool isAvailable() const override;
    QString version() const override;
    int abiVersion() const override;
    QString loadStatus() const override;
    QString libraryPath() const;
    CoreRuntimeState state() const override;

    CoreOperationResult validate(const CoreLaunchRequest& request) override;
    CoreOperationResult start(const CoreLaunchRequest& request) override;
    CoreOperationResult stop() override;

private:
    using AbiVersionFunction = int (*)();
    using StringFunction = char* (*)();
    using ConfigFunction = char* (*)(const char*, std::size_t);
    using StateFunction = int (*)();
    using FreeFunction = void (*)(void*);

    void loadBridge();
    void drainLogs();
    void setState(CoreRuntimeState state);
    CoreRuntimeState queryState() const;
    CoreOperationResult callConfigFunction(ConfigFunction function,
                                           const CoreLaunchRequest& request,
                                           const QString& errorCode);
    CoreOperationResult takeResult(char* result, const QString& errorCode);
    QString takeString(char* value) const;
    QString sanitize(const QString& message) const;

    CoreRuntimeCoordinator* m_coordinator = nullptr;
    QLibrary m_library;
    QTimer m_logTimer;
    QString m_libraryPath;
    QString m_loadStatus;
    QString m_version;
    CoreRuntimeState m_state = CoreRuntimeState::Stopped;
    int m_abiVersion = 0;
    bool m_available = false;
    bool m_coordinatorAcquired = false;

    AbiVersionFunction m_abiVersionFunction = nullptr;
    StringFunction m_versionFunction = nullptr;
    ConfigFunction m_validateFunction = nullptr;
    ConfigFunction m_startFunction = nullptr;
    StringFunction m_stopFunction = nullptr;
    StateFunction m_stateFunction = nullptr;
    StringFunction m_drainLogsFunction = nullptr;
    FreeFunction m_freeFunction = nullptr;
};

} // namespace zarya
