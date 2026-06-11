#include "service/macos/MacHelperServiceManager.h"

#include "app/BuildInfo.h"
#include "service/HelperServiceIdentity.h"

namespace zarya {

MacHelperServiceManager::MacHelperServiceManager(QObject* parent)
    : IHelperServiceManager(parent)
{
}

QString MacHelperServiceManager::backendName() const
{
    return QStringLiteral("macOS SMAppService design");
}

bool MacHelperServiceManager::isSupported() const
{
    return false;
}

HelperServiceStatus MacHelperServiceManager::status()
{
    HelperServiceStatus result;
    result.backend = backendName();
    result.serviceName = HelperServiceIdentity::macOsLabel();
    result.version = BuildInfo::appVersion();
    result.state = HelperServiceInstallState::DesignOnly;
    result.warnings.append(QStringLiteral(
        "Production helper installation requires signed/notarized app and ServiceManagement "
        "integration. See docs/service/macos-smappservice.md."));
    return result;
}

bool MacHelperServiceManager::install(const HelperServiceInstallOptions&, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "macOS privileged helper installation is design-only in 0.28. %1")
                            .arg(manualInstallCommand({}));
    }
    return false;
}

bool MacHelperServiceManager::uninstall(bool, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("macOS helper service uninstall is not available in 0.28.");
    }
    return false;
}

bool MacHelperServiceManager::start(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("macOS helper service start is not available in 0.28.");
    }
    return false;
}

bool MacHelperServiceManager::stop(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("macOS helper service stop is not available in 0.28.");
    }
    return false;
}

bool MacHelperServiceManager::restart(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("macOS helper service restart is not available in 0.28.");
    }
    return false;
}

QString MacHelperServiceManager::recoveryInstructions() const
{
    return QStringLiteral(
        "macOS helper service recovery is not installed in 0.28.\n"
        "Future design note:\n"
        "  sudo launchctl bootout system/%1\n\n"
        "For now, stop any manually launched zarya-helper and recover kill switch if needed.")
        .arg(HelperServiceIdentity::macOsLabel());
}

QString MacHelperServiceManager::manualInstallCommand(const HelperServiceInstallOptions&) const
{
    return QStringLiteral("Not available in 0.28. See docs/service/macos-smappservice.md.");
}

} // namespace zarya
