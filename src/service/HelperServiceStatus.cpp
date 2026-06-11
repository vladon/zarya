#include "service/HelperServiceStatus.h"

namespace zarya {

QString helperServiceInstallStateToString(HelperServiceInstallState state)
{
    switch (state) {
    case HelperServiceInstallState::NotInstalled:
        return QStringLiteral("NotInstalled");
    case HelperServiceInstallState::Installed:
        return QStringLiteral("Installed");
    case HelperServiceInstallState::Stopped:
        return QStringLiteral("Stopped");
    case HelperServiceInstallState::Running:
        return QStringLiteral("Running");
    case HelperServiceInstallState::Failed:
        return QStringLiteral("Failed");
    case HelperServiceInstallState::Unsupported:
        return QStringLiteral("Unsupported");
    case HelperServiceInstallState::DesignOnly:
        return QStringLiteral("DesignOnly");
    case HelperServiceInstallState::Unknown:
        break;
    }
    return QStringLiteral("Unknown");
}

} // namespace zarya
