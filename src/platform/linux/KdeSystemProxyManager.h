#pragma once

#include "platform/ISystemProxyManager.h"
#include "platform/PlatformProcessUtils.h"

namespace zarya {

class KdeSystemProxyManager : public ISystemProxyManager {
public:
    explicit KdeSystemProxyManager(
        PlatformProcessRunner processRunner = defaultPlatformProcessRunner());

    bool isSupported() const override;
    SystemProxyState readCurrentState(QString* errorMessage = nullptr) override;
    bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) override;
    bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) override;

    QString backendName() const override;
    QString supportLevel() const override;
    QString limitations() const override;

private:
    struct ConfigTools {
        QString read;
        QString write;

        bool isValid() const { return !read.isEmpty() && !write.isEmpty(); }
    };

    ConfigTools detectConfigTools() const;
    bool readConfigValue(const ConfigTools& tools, const QString& key, bool* present,
                         QString* value, QString* errorMessage) const;
    bool writeConfigValue(const ConfigTools& tools, const QString& key, bool present,
                          const QString& value, QString* errorMessage) const;
    bool writeSnapshot(const SystemProxyState& state, QString* errorMessage) const;
    bool reloadKio(QString* errorMessage) const;

    PlatformProcessRunner m_processRunner;
};

} // namespace zarya
