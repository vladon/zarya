#pragma once

#include "killswitch/KillSwitchManager.h"

namespace zarya {

class StubKillSwitchManager : public IKillSwitchBackend {
public:
    explicit StubKillSwitchManager(QString platformId);

    QString backendId() const override;
    QString displayName() const override;
    KillSwitchState checkSupport(bool privileged) const override;
    bool enable(const KillSwitchRuleSet& rules, QString* errorMessage) override;
    bool disable(QString* errorMessage) override;
    QString recoveryInstructions() const override;

private:
    QString m_platformId;
};

} // namespace zarya
