#pragma once

#include "killswitch/KillSwitchManager.h"

namespace zarya {

class WindowsKillSwitchManager : public IKillSwitchBackend {
public:
    QString backendId() const override;
    QString displayName() const override;
    KillSwitchState checkSupport(bool privileged) const override;
    bool enable(const KillSwitchRuleSet& rules, QString* errorMessage) override;
    bool disable(QString* errorMessage) override;
    QString recoveryInstructions() const override;
};

} // namespace zarya
