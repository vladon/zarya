#pragma once

#include "runtime/IRuntimeBackend.h"

class QWidget;

namespace zarya {

class CoreManager;

class SingBoxTunRuntimeBackend : public IRuntimeBackend {
    Q_OBJECT

public:
    explicit SingBoxTunRuntimeBackend(CoreManager* coreManager, QObject* parent = nullptr);

    void setDialogParent(QWidget* parent);

    QString displayName() const override;
    RuntimeBackendType type() const override;

    bool isSupported(QString* reason = nullptr) const override;
    bool validateProfile(const Profile& profile, QString* reason = nullptr) const override;

    bool start(const Profile& profile, const RuntimeStartOptions& options) override;
    bool stop() override;
    bool isRunning() const override;

private:
    bool confirmPrivilegeWarnings(const RuntimeStartOptions& options);

    CoreManager* m_coreManager = nullptr;
    QWidget* m_dialogParent = nullptr;
    RuntimeState m_state = RuntimeState::Stopped;
};

} // namespace zarya
