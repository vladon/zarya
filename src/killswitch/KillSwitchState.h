#pragma once

#include "killswitch/KillSwitchMode.h"

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace zarya {

enum class KillSwitchStatus {
    Disabled,
    Enabling,
    Enabled,
    Disabling,
    Failed,
    NeedsRecovery,
    Unsupported,
};

QString killSwitchStatusToString(KillSwitchStatus status);

struct KillSwitchState {
    KillSwitchStatus status = KillSwitchStatus::Disabled;
    KillSwitchMode mode = KillSwitchMode::Disabled;
    QString backend;
    QString lastError;
    QDateTime enabledAt;
    QStringList activeRules;
    bool recoveryMarkerPresent = false;
    bool privileged = false;
    bool supported = false;
};

} // namespace zarya
