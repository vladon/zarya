#include "killswitch/KillSwitchState.h"

namespace zarya {

QString killSwitchStatusToString(KillSwitchStatus status)
{
    switch (status) {
    case KillSwitchStatus::Disabled:
        return QStringLiteral("disabled");
    case KillSwitchStatus::Enabling:
        return QStringLiteral("enabling");
    case KillSwitchStatus::Enabled:
        return QStringLiteral("enabled");
    case KillSwitchStatus::Disabling:
        return QStringLiteral("disabling");
    case KillSwitchStatus::Failed:
        return QStringLiteral("failed");
    case KillSwitchStatus::NeedsRecovery:
        return QStringLiteral("needs-recovery");
    case KillSwitchStatus::Unsupported:
        return QStringLiteral("unsupported");
    }
    return QStringLiteral("unknown");
}

} // namespace zarya
