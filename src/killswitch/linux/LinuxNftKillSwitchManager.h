#pragma once

#include "killswitch/KillSwitchManager.h"

namespace zarya {

class LinuxNftKillSwitchManager : public IKillSwitchBackend {
public:
    QString backendId() const override;
    QString displayName() const override;
    KillSwitchState checkSupport(bool privileged) const override;
    bool enable(const KillSwitchRuleSet& rules, QString* errorMessage) override;
    bool disable(QString* errorMessage) override;
    QString recoveryInstructions() const override;

private:
    QString rulesFilePath() const;
    QString buildRulesFile(const KillSwitchRuleSet& rules, QString* errorMessage) const;
    bool runNft(const QStringList& arguments, QString* output, QString* errorMessage) const;
};

} // namespace zarya
