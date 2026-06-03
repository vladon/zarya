#pragma once

#include "platform/ISystemProxyManager.h"

namespace zarya {

class GnomeSystemProxyManager : public ISystemProxyManager {
public:
    bool isSupported() const override;
    SystemProxyState readCurrentState(QString* errorMessage = nullptr) override;
    bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) override;
    bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) override;

    QString backendName() const override;
    QString supportLevel() const override;
    QString limitations() const override;

private:
    QString readGsettingsValue(const QString& schemaKey, QString* errorMessage) const;
    bool setGsettingsValue(const QString& schemaKey, const QString& rawValue,
                           QString* errorMessage) const;
    bool setGsettingsValue(const QString& schemaKey, int value, QString* errorMessage) const;
};

} // namespace zarya
