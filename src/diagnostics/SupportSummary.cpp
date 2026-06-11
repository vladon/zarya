#include "diagnostics/SupportSummary.h"

#include "app/BuildInfo.h"
#include "cores/CoreBinaryManager.h"
#include "cores/CoreInfo.h"
#include "diagnostics/DiagnosticsRedactor.h"
#include "domain/CoreType.h"
#include "helperclient/HelperProcessManager.h"
#include "killswitch/KillSwitchMode.h"
#include "packaging/InstallationMode.h"
#include "packaging/PackagingInfo.h"
#include "platform/SystemProxyController.h"
#include "runtime/RuntimeBackendType.h"
#include "service/HelperServiceStatus.h"
#include "service/IHelperServiceManager.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QSysInfo>

namespace zarya {

namespace {

QString coreVersionLine(const DiagnosticsContext& context, CoreType type)
{
    if (!context.coreBinaryManager) {
        return QStringLiteral("unknown");
    }
    const CoreInfo info = context.coreBinaryManager->infoFor(type);
    if (!info.exists) {
        return QStringLiteral("not installed");
    }
    if (info.installedVersion.isEmpty()) {
        return QStringLiteral("installed (version unknown)");
    }
    return info.installedVersion;
}

QString helperSummary(const DiagnosticsContext& context)
{
    if (context.helperService) {
        const HelperServiceStatus status = context.helperService->status();
        if (status.state == HelperServiceInstallState::Running) {
            return QStringLiteral("service running");
        }
        if (status.state == HelperServiceInstallState::Installed
            || status.state == HelperServiceInstallState::Stopped) {
            return QStringLiteral("service installed");
        }
    }
    if (context.helper
        && context.helper->connectionState() == HelperConnectionState::Connected) {
        return QStringLiteral("connected");
    }
    return QStringLiteral("not installed");
}

QString redactSummaryLine(const QString& line)
{
    return DiagnosticsRedactor::redactText(line, DiagnosticsRedactionMode::Strict);
}

} // namespace

QString SupportSummary::buildClipboardText(const DiagnosticsContext& context)
{
    const AppSettings& settings = AppSettings::instance();
    QString platform;
#if defined(Q_OS_WIN)
    platform = QStringLiteral("Windows %1 %2").arg(QSysInfo::productVersion(),
                                                  QSysInfo::currentCpuArchitecture());
#elif defined(Q_OS_MACOS)
    platform = QStringLiteral("macOS %1 %2").arg(QSysInfo::productVersion(),
                                                 QSysInfo::currentCpuArchitecture());
#elif defined(Q_OS_LINUX)
    platform = QStringLiteral("Linux %1 %2").arg(QSysInfo::productVersion(),
                                               QSysInfo::currentCpuArchitecture());
#else
    platform = QStringLiteral("%1 %2").arg(QSysInfo::prettyProductName(),
                                           QSysInfo::currentCpuArchitecture());
#endif

    QString runtime = runtimeModeToString(settings.effectiveRuntimeMode());
    if (settings.effectiveRuntimeMode() == RuntimeMode::SystemProxyXray) {
        runtime = QStringLiteral("Xray system proxy");
    } else if (settings.effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental) {
        runtime = QStringLiteral("sing-box TUN experimental");
    }

    const QString proxyBackend =
        context.systemProxy ? context.systemProxy->backendName() : QStringLiteral("unknown");

    const QString killSwitch =
        settings.enableExperimentalKillSwitch() ? QStringLiteral("enabled (experimental)")
                                              : QStringLiteral("disabled");

    QStringList lines;
    lines << QStringLiteral("Zarya: %1").arg(BuildInfo::appVersion());
    lines << QStringLiteral("Installation: %1").arg(InstallationInfo::currentModeString());
    lines << QStringLiteral("App update channel: %1").arg(settings.appUpdateChannelKey());
    lines << QStringLiteral("Platform: %1").arg(redactSummaryLine(platform));
    lines << QStringLiteral("Runtime: %1").arg(runtime);
    lines << QStringLiteral("Core:");
    lines << QStringLiteral("  Xray: %1").arg(coreVersionLine(context, CoreType::Xray));
    lines << QStringLiteral("  sing-box: %1").arg(coreVersionLine(context, CoreType::SingBox));
    lines << QStringLiteral("System proxy backend: %1").arg(proxyBackend);
    lines << QStringLiteral("Helper: %1").arg(helperSummary(context));
    lines << QStringLiteral("Kill switch: %1").arg(killSwitch);
    lines << QStringLiteral("Portable mode: %1")
                 .arg(AppPaths::isPortableMode() ? QStringLiteral("true")
                                                 : QStringLiteral("false"));

    return redactSummaryLine(lines.join(QStringLiteral("\n")));
}

bool SupportSummary::copyToClipboard(const DiagnosticsContext& context)
{
    if (!QGuiApplication::clipboard()) {
        return false;
    }
    QGuiApplication::clipboard()->setText(buildClipboardText(context));
    return true;
}

} // namespace zarya
