#include "service/HelperServiceInstallOptions.h"

#include "service/HelperServiceIdentity.h"
#include "storage/AppPaths.h"

namespace zarya {

HelperServiceInstallOptions HelperServiceInstallOptions::defaultsForCurrentApp(
    const QString& helperExecutablePath)
{
    AppPaths::initialize(AppPaths::isPortableMode());
    HelperServiceInstallOptions options;
    options.helperExecutablePath = helperExecutablePath;
    options.serviceName = HelperServiceIdentity::internalServiceName();
    options.displayName = HelperServiceIdentity::displayName();
    options.allowedRuntimeDir = AppPaths::runtimeDir();
    options.allowedCoreDir = AppPaths::singBoxCoreDir();
    options.logFilePath = AppPaths::runtimeDir() + QStringLiteral("/helper-service.log");
    options.manualStart = true;
    options.startAfterInstall = false;
    return options;
}

} // namespace zarya
