#pragma once

#include "platform/ISystemProxyManager.h"

namespace zarya {

class StubSystemProxyManager : public ISystemProxyManager {
public:
    explicit StubSystemProxyManager(QString reason = {});

    bool isSupported() const override;
    SystemProxyState readCurrentState(QString* errorMessage = nullptr) override;
    bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) override;
    bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) override;

    QString backendName() const override;
    QString supportLevel() const override;
    QString limitations() const override;

private:
    QString m_reason;
};

} // namespace zarya
