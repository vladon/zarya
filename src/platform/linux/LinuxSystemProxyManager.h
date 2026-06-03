#pragma once

#include "platform/ISystemProxyManager.h"

#include <memory>

namespace zarya {

class LinuxSystemProxyManager : public ISystemProxyManager {
public:
    LinuxSystemProxyManager();

    bool isSupported() const override;
    SystemProxyState readCurrentState(QString* errorMessage = nullptr) override;
    bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) override;
    bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) override;

    QString backendName() const override;
    QString supportLevel() const override;
    QString limitations() const override;

    QString detectedDesktopName() const;

private:
    std::unique_ptr<ISystemProxyManager> m_backend;
    QString m_detectedDesktop;
};

} // namespace zarya
