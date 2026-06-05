#include "killswitch/stub/StubKillSwitchManager.h"

namespace zarya {

StubKillSwitchManager::StubKillSwitchManager(QString platformId)
    : m_platformId(std::move(platformId))
{
}

QString StubKillSwitchManager::backendId() const
{
    return QStringLiteral("stub-%1").arg(m_platformId);
}

QString StubKillSwitchManager::displayName() const
{
    return QStringLiteral("Kill switch stub");
}

KillSwitchState StubKillSwitchManager::checkSupport(bool privileged) const
{
    KillSwitchState state;
    state.backend = backendId();
    state.privileged = privileged;
    state.supported = false;
    state.status = KillSwitchStatus::Unsupported;
    state.lastError = QStringLiteral("Kill switch is not supported on this platform.");
    return state;
}

bool StubKillSwitchManager::enable(const KillSwitchRuleSet& rules, QString* errorMessage)
{
    Q_UNUSED(rules);
    if (errorMessage) {
        *errorMessage = QStringLiteral("Kill switch is not supported on this platform.");
    }
    return false;
}

bool StubKillSwitchManager::disable(QString* errorMessage)
{
    Q_UNUSED(errorMessage);
    return true;
}

QString StubKillSwitchManager::recoveryInstructions() const
{
    return QStringLiteral("No platform-specific kill switch rules were installed.");
}

} // namespace zarya
