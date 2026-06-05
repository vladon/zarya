#pragma once

#include "runtime/IRuntimeBackend.h"

#include <memory>

class QWidget;

namespace zarya {

class CoreManager;
class HelperProcessManager;

class SingBoxTunRuntimeBackend : public IRuntimeBackend {
    Q_OBJECT

public:
    explicit SingBoxTunRuntimeBackend(CoreManager* coreManager, QObject* parent = nullptr);
    ~SingBoxTunRuntimeBackend() override;

    void setDialogParent(QWidget* parent);

    HelperProcessManager* helperManager();

    QString displayName() const override;
    RuntimeBackendType type() const override;

    bool isSupported(QString* reason = nullptr) const override;
    bool validateProfile(const Profile& profile, QString* reason = nullptr) const override;

    bool start(const Profile& profile, const RuntimeStartOptions& options) override;
    bool stop() override;
    bool isRunning() const override;

private:
    bool confirmPrivilegeWarnings(const RuntimeStartOptions& options);
    bool writeTunConfig(const Profile& profile, const RuntimeStartOptions& options,
                        QString* configPath, QString* errorMessage);
    bool startDirect(const Profile& profile, const QString& configPath);
    bool startViaHelper(const Profile& profile, const QString& configPath);
    bool stopDirect();
    bool stopViaHelper();

    bool wantsKillSwitch() const;
    bool ensureHelperConnected(QString* errorMessage);
    bool enableKillSwitchViaHelper(const Profile& profile, QString* errorMessage);
    bool disableKillSwitchViaHelper(QString* errorMessage);

    CoreManager* m_coreManager = nullptr;
    QWidget* m_dialogParent = nullptr;
    std::unique_ptr<HelperProcessManager> m_helperManager;
    RuntimeState m_state = RuntimeState::Stopped;
    bool m_runningViaHelper = false;
    bool m_killSwitchSessionActive = false;
};

} // namespace zarya
