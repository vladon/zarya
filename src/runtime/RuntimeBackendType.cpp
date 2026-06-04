#include "runtime/RuntimeBackendType.h"

namespace zarya {

QString runtimeModeToString(RuntimeMode mode)
{
    switch (mode) {
    case RuntimeMode::SystemProxyXray:
        return QStringLiteral("system-proxy-xray");
    case RuntimeMode::TunSingBoxExperimental:
        return QStringLiteral("tun-singbox-experimental");
    }
    return QStringLiteral("system-proxy-xray");
}

RuntimeMode runtimeModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("tun-singbox-experimental")) {
        return RuntimeMode::TunSingBoxExperimental;
    }
    return RuntimeMode::SystemProxyXray;
}

QString runtimeModeDisplayString(RuntimeMode mode)
{
    switch (mode) {
    case RuntimeMode::SystemProxyXray:
        return QStringLiteral("Xray system proxy");
    case RuntimeMode::TunSingBoxExperimental:
        return QStringLiteral("sing-box TUN experimental");
    }
    return QStringLiteral("Xray system proxy");
}

QString runtimeStateDisplayString(RuntimeState state)
{
    switch (state) {
    case RuntimeState::Stopped:
        return QStringLiteral("stopped");
    case RuntimeState::Starting:
        return QStringLiteral("starting");
    case RuntimeState::Running:
        return QStringLiteral("running");
    case RuntimeState::Stopping:
        return QStringLiteral("stopping");
    case RuntimeState::Error:
        return QStringLiteral("error");
    }
    return QStringLiteral("stopped");
}

} // namespace zarya
