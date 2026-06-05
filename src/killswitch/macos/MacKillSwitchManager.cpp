#include "killswitch/macos/MacKillSwitchManager.h"

namespace zarya {

QString MacKillSwitchManager::backendId() const
{
    return QStringLiteral("macos-pf-stub");
}

QString MacKillSwitchManager::displayName() const
{
    return QStringLiteral("macOS PF stub (unsupported)");
}

KillSwitchState MacKillSwitchManager::checkSupport(bool privileged) const
{
    KillSwitchState state;
    state.backend = backendId();
    state.privileged = privileged;
    state.supported = false;
    state.status = KillSwitchStatus::Unsupported;
    state.lastError = QStringLiteral(
        "macOS kill switch is unsupported in 0.16. PF is not a stable public API for third-party "
        "apps; production should use Network Extension.");
    return state;
}

bool MacKillSwitchManager::enable(const KillSwitchRuleSet& rules, QString* errorMessage)
{
    Q_UNUSED(rules);
    if (errorMessage) {
        *errorMessage = QStringLiteral("macOS kill switch is unsupported in 0.16.");
    }
    return false;
}

bool MacKillSwitchManager::disable(QString* errorMessage)
{
    Q_UNUSED(errorMessage);
    return true;
}

QString MacKillSwitchManager::recoveryInstructions() const
{
    return QStringLiteral(
        "macOS recovery (0.16):\n\n"
        "Zarya does not install PF rules in this milestone.\n"
        "Future production path: Network Extension or explicit user opt-in PF anchor.");
}

} // namespace zarya
