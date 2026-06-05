#include "killswitch/KillSwitchMode.h"

namespace zarya {

QString killSwitchModeToString(KillSwitchMode mode)
{
    switch (mode) {
    case KillSwitchMode::TunOnlyExperimental:
        return QStringLiteral("tun-only-experimental");
    case KillSwitchMode::Disabled:
        break;
    }
    return QStringLiteral("disabled");
}

KillSwitchMode killSwitchModeFromString(const QString& value)
{
    if (value == QStringLiteral("tun-only-experimental")) {
        return KillSwitchMode::TunOnlyExperimental;
    }
    return KillSwitchMode::Disabled;
}

} // namespace zarya
