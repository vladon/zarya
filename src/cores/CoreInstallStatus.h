#pragma once

#include <QString>

namespace zarya {

enum class CoreInstallStatus {
    Missing,
    Installed,
    External,
    Running,
    UpdateAvailable,
    Updating,
    Failed,
    Unknown,
};

QString coreInstallStatusToString(CoreInstallStatus status);

} // namespace zarya
