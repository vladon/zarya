#pragma once

#include "platform/SystemProxyState.h"

#include <QString>

namespace zarya {

class ISystemProxyManager {
public:
    virtual ~ISystemProxyManager() = default;

    virtual bool isSupported() const = 0;
    virtual SystemProxyState readCurrentState(QString* errorMessage = nullptr) = 0;
    virtual bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) = 0;
    virtual bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) = 0;
};

} // namespace zarya
