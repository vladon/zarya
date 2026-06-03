#pragma once

#include "platform/ISystemProxyManager.h"

namespace zarya {

class StubSystemProxyManager : public ISystemProxyManager {
public:
    bool isSupported() const override;
    SystemProxyState readCurrentState(QString* errorMessage = nullptr) override;
    bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) override;
    bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) override;
};

} // namespace zarya
