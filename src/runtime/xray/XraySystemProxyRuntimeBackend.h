#pragma once

#include "runtime/IRuntimeBackend.h"

#include <functional>

namespace zarya {

class XraySystemProxyRuntimeBackend : public IRuntimeBackend {
    Q_OBJECT

public:
    explicit XraySystemProxyRuntimeBackend(QObject* parent = nullptr);

    void setStartHandler(std::function<bool(const Profile&, const RuntimeStartOptions&)> handler);
    void setStopHandler(std::function<bool()> handler);
    void setRunningHandler(std::function<bool()> handler);

    QString displayName() const override;
    RuntimeBackendType type() const override;

    bool isSupported(QString* reason = nullptr) const override;
    bool validateProfile(const Profile& profile, QString* reason = nullptr) const override;

    bool start(const Profile& profile, const RuntimeStartOptions& options) override;
    bool stop() override;
    bool isRunning() const override;

private:
    std::function<bool(const Profile&, const RuntimeStartOptions&)> m_startHandler;
    std::function<bool()> m_stopHandler;
    std::function<bool()> m_runningHandler;
};

} // namespace zarya
