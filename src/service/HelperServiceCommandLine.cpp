#include "service/HelperServiceCommandLine.h"

#include "service/HelperServiceIdentity.h"

#include <QDir>

namespace zarya {

QString HelperServiceCommandLine::buildBinaryPath(const HelperServiceInstallOptions& options)
{
    const QString helper = QDir::toNativeSeparators(options.helperExecutablePath);
    const QString runtime = QDir::toNativeSeparators(options.allowedRuntimeDir);
    const QString core = QDir::toNativeSeparators(options.allowedCoreDir);
    const QString logFile = QDir::toNativeSeparators(options.logFilePath);
    const QString serviceName =
        options.serviceName.isEmpty() ? HelperServiceIdentity::internalServiceName()
                                      : options.serviceName;

    QStringList parts = {QStringLiteral("\"%1\"").arg(helper),
                         QStringLiteral("--service"),
                         QStringLiteral("--ipc-mode"),
                         QStringLiteral("local-socket"),
                         QStringLiteral("--service-name"),
                         serviceName,
                         QStringLiteral("--allowed-runtime-dir"),
                         QStringLiteral("\"%1\"").arg(runtime),
                         QStringLiteral("--allowed-core-dir"),
                         QStringLiteral("\"%1\"").arg(core)};
    if (!logFile.isEmpty()) {
        parts << QStringLiteral("--log-file") << QStringLiteral("\"%1\"").arg(logFile);
    }
    if (!options.allowedUserId.isEmpty()) {
        parts << QStringLiteral("--allowed-client-sid") << options.allowedUserId;
    }
    return parts.join(QLatin1Char(' '));
}

} // namespace zarya
