#pragma once

#include "platform/ISystemProxyManager.h"
#include "platform/SystemProxyState.h"

#include <QString>
#include <functional>
#include <memory>

namespace zarya {

enum class SystemProxyUiStatus {
    Unsupported,
    Partial,
    Off,
    On,
    Failed,
};

enum class SystemProxyRestoreMode {
    Automatic,
    Manual,
};

class SystemProxyController {
public:
    SystemProxyController();

    bool isSupported() const;
    QString backendName() const;
    QString supportLevel() const;
    QString limitations() const;
    SystemProxyUiStatus uiStatus() const;
    QString uiStatusText() const;
    bool hasSavedState() const;
    bool enabledByZarya() const;
    QString lastError() const;

    void logCurrentState(const std::function<void(const QString&)>& logLine) const;

    bool enableLocalHttpProxy(int port, const std::function<void(const QString&)>& logLine,
                              QString* errorMessage = nullptr);
    bool restorePreviousProxy(SystemProxyRestoreMode mode,
                              const std::function<void(const QString&)>& logLine,
                              QString* errorMessage = nullptr);
    bool restorePersistedPreviousProxy(const std::function<void(const QString&)>& logLine,
                                       QString* errorMessage = nullptr);
    bool tryClearZaryaOwnedProxy(int httpPort,
                                 const std::function<void(const QString&)>& logLine,
                                 QString* errorMessage = nullptr);

    void clearRuntimeState();

private:
    bool ensurePreviousStateSaved(const std::function<void(const QString&)>& logLine,
                                 QString* errorMessage);

    std::unique_ptr<ISystemProxyManager> m_manager;
    SystemProxyState m_savedState;
    bool m_hasSavedState = false;
    bool m_enabledByZarya = false;
    SystemProxyUiStatus m_uiStatus = SystemProxyUiStatus::Unsupported;
    QString m_lastError;
};

} // namespace zarya
