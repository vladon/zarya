#include "cores/CoreInstallStatus.h"

namespace zarya {

QString coreInstallStatusToString(CoreInstallStatus status)
{
    switch (status) {
    case CoreInstallStatus::Missing:
        return QStringLiteral("Missing");
    case CoreInstallStatus::Installed:
        return QStringLiteral("Installed");
    case CoreInstallStatus::External:
        return QStringLiteral("External");
    case CoreInstallStatus::Running:
        return QStringLiteral("Running");
    case CoreInstallStatus::UpdateAvailable:
        return QStringLiteral("Update available");
    case CoreInstallStatus::Updating:
        return QStringLiteral("Updating");
    case CoreInstallStatus::Failed:
        return QStringLiteral("Failed");
    case CoreInstallStatus::Unknown:
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

} // namespace zarya
