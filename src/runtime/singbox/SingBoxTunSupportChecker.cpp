#include "runtime/singbox/SingBoxTunSupportChecker.h"

#include "platform/PlatformPrivilege.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace zarya {

TunSupportResult SingBoxTunSupportChecker::check(const QString& singBoxExecutablePath)
{
    TunSupportResult result;
#if defined(Q_OS_WIN)
    result.platform = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    result.platform = QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
    result.platform = QStringLiteral("linux");
#else
    result.platform = QStringLiteral("unknown");
#endif

    const QString executable = singBoxExecutablePath.trimmed();
    if (executable.isEmpty() || !QFileInfo::exists(executable)) {
        result.reason = QStringLiteral("sing-box executable path is not configured.");
        return result;
    }

    result.supported = true;

    const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
    result.hasRequiredPrivileges = privileges.elevated;
    result.warnings.append(
        QStringLiteral("TUN mode is experimental; kill switch is not implemented."));
    result.warnings.append(QStringLiteral(
        "Some Xray routing/DNS features may not map exactly to sing-box; see warnings before "
        "start."));

#if defined(Q_OS_WIN)
    result.warnings.append(QStringLiteral("TUN may require administrator privileges."));
    result.warnings.append(
        QStringLiteral("Wintun driver may be required depending on your sing-box build."));
    if (!privileges.elevated) {
        result.warnings.append(QStringLiteral("Current process is not elevated."));
    }
#elif defined(Q_OS_MACOS)
    result.supported = true;
    result.warnings.append(QStringLiteral("TUN may require elevated privileges."));
    result.warnings.append(QStringLiteral(
        "Production macOS VPN-like mode may require Network Extension packaging (not in 0.13)."));
#elif defined(Q_OS_LINUX)
    if (!QFile::exists(QStringLiteral("/dev/net/tun"))) {
        result.supported = false;
        result.reason = QStringLiteral("TUN device is not available: /dev/net/tun missing.");
        return result;
    }
    if (!privileges.elevated) {
        result.warnings.append(
            QStringLiteral("TUN route changes often require root or elevated capabilities."));
    }
    if (QStandardPaths::findExecutable(QStringLiteral("ip")).isEmpty()) {
        result.warnings.append(QStringLiteral("iproute2 (ip command) was not found in PATH."));
    }
#endif

    return result;
}

} // namespace zarya
