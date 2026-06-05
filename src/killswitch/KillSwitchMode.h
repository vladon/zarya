#pragma once

#include <QString>

namespace zarya {

enum class KillSwitchMode {
    Disabled,
    TunOnlyExperimental,
};

QString killSwitchModeToString(KillSwitchMode mode);
KillSwitchMode killSwitchModeFromString(const QString& value);

} // namespace zarya
