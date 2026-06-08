#include "killswitch/windows/WindowsWfpKillSwitchManager.h"

namespace zarya {

QString WindowsWfpKillSwitchManager::backendId() const
{
    return QStringLiteral("windows-wfp");
}

QString WindowsWfpKillSwitchManager::displayName() const
{
    return QStringLiteral("Windows WFP PoC");
}

KillSwitchState WindowsWfpKillSwitchManager::checkSupport(bool /*privileged*/) const
{
    KillSwitchState state;
    state.backend = backendId();
    state.supported = false;
    state.status = KillSwitchStatus::Unsupported;
    state.lastError = QStringLiteral("Windows WFP kill switch is only built on Windows.");
    return state;
}

bool WindowsWfpKillSwitchManager::enable(const KillSwitchRuleSet& /*rules*/, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Windows WFP kill switch is only built on Windows.");
    }
    return false;
}

bool WindowsWfpKillSwitchManager::disable(QString* /*errorMessage*/)
{
    return true;
}

QString WindowsWfpKillSwitchManager::recoveryInstructions() const
{
    return QStringLiteral(
        "Windows WFP kill switch recovery:\n\n"
        "Preferred:\n"
        "  zarya-helper --recover-killswitch\n\n"
        "From Zarya GUI:\n"
        "  Settings → Kill Switch → Disable Now\n\n"
        "Manual inspection:\n"
        "  netsh wfp show state\n\n"
        "Zarya only removes its own provider/sublayer filters. Do not delete unrelated WFP "
        "objects.");
}

void WindowsWfpKillSwitchManager::augmentMarker(KillSwitchMarkerData* /*data*/) const
{
}

QStringList WindowsWfpKillSwitchManager::activeRuleDescriptions() const
{
    return {};
}

} // namespace zarya
