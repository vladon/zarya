#include "killswitch/windows/WindowsKillSwitchManager.h"

namespace zarya {

QString WindowsKillSwitchManager::backendId() const
{
    return QStringLiteral("windows-wfp-stub");
}

QString WindowsKillSwitchManager::displayName() const
{
    return QStringLiteral("Windows WFP design stub");
}

KillSwitchState WindowsKillSwitchManager::checkSupport(bool privileged) const
{
    KillSwitchState state;
    state.backend = backendId();
    state.privileged = privileged;
    state.supported = false;
    state.status = KillSwitchStatus::Unsupported;
    state.lastError = QStringLiteral(
        "Windows kill switch is a design stub in 0.16. Production should use Windows Filtering "
        "Platform (WFP). No firewall rules are installed.");
    return state;
}

bool WindowsKillSwitchManager::enable(const KillSwitchRuleSet& rules, QString* errorMessage)
{
    Q_UNUSED(rules);
    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "Windows kill switch is not implemented in 0.16 (WFP backend planned).");
    }
    return false;
}

bool WindowsKillSwitchManager::disable(QString* errorMessage)
{
    Q_UNUSED(errorMessage);
    return true;
}

QString WindowsKillSwitchManager::recoveryInstructions() const
{
    return QStringLiteral(
        "Windows recovery (0.16):\n\n"
        "No Zarya kill switch firewall rules are installed by default in this milestone.\n"
        "Future versions may create rules under group \"Zarya Kill Switch\".\n\n"
        "Production kill switch should use Windows Filtering Platform (WFP).");
}

} // namespace zarya
