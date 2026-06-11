#include "service/stub/StubHelperServiceManager.h"

namespace zarya {

StubHelperServiceManager::StubHelperServiceManager(QObject* parent)
    : IHelperServiceManager(parent)
{
}

QString StubHelperServiceManager::backendName() const
{
    return QStringLiteral("Manual helper");
}

bool StubHelperServiceManager::isSupported() const
{
    return false;
}

HelperServiceStatus StubHelperServiceManager::status()
{
    HelperServiceStatus result;
    result.state = HelperServiceInstallState::Unsupported;
    result.backend = backendName();
    result.warnings.append(QStringLiteral("Privileged helper service is not available on this platform."));
    return result;
}

bool StubHelperServiceManager::install(const HelperServiceInstallOptions&, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Helper service installation is not supported on this platform.");
    }
    return false;
}

bool StubHelperServiceManager::uninstall(bool, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Helper service uninstall is not supported on this platform.");
    }
    return false;
}

bool StubHelperServiceManager::start(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Helper service start is not supported on this platform.");
    }
    return false;
}

bool StubHelperServiceManager::stop(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Helper service stop is not supported on this platform.");
    }
    return false;
}

bool StubHelperServiceManager::restart(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Helper service restart is not supported on this platform.");
    }
    return false;
}

QString StubHelperServiceManager::recoveryInstructions() const
{
    return QStringLiteral("Stop the manually launched zarya-helper process and recover kill switch from "
                          "Settings → Experimental if needed.");
}

QString StubHelperServiceManager::manualInstallCommand(const HelperServiceInstallOptions&) const
{
    return QStringLiteral("Not supported on this platform.");
}

} // namespace zarya
