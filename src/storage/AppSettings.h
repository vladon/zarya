#pragma once

#include <QString>

namespace zarya {

class AppSettings {
public:
    static AppSettings& instance();

    QString xrayExecutablePath() const;
    void setXrayExecutablePath(const QString& path);

    int socksPort() const;
    void setSocksPort(int port);

    int httpPort() const;
    void setHttpPort(int port);

    QString resolvedXrayPath() const;

    bool autoEnableSystemProxyOnStart() const;
    void setAutoEnableSystemProxyOnStart(bool enabled);

    bool restoreProxyOnExit() const;
    void setRestoreProxyOnExit(bool enabled);

    bool confirmBeforeChangingSystemProxy() const;
    void setConfirmBeforeChangingSystemProxy(bool enabled);

    static bool defaultAutoEnableSystemProxyOnStart();

private:
    AppSettings() = default;
};

} // namespace zarya
