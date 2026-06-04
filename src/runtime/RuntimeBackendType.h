#pragma once

#include <QString>

namespace zarya {

enum class RuntimeMode {
    SystemProxyXray,
    TunSingBoxExperimental,
};

enum class TunDnsHijackMode {
    HijackToSingBoxDns,
    Disabled,
};

enum class RuntimeBackendType {
    XraySystemProxy,
    SingBoxTunExperimental,
};

enum class RuntimeState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Error,
};

QString runtimeModeToString(RuntimeMode mode);
RuntimeMode runtimeModeFromString(const QString& value);
QString runtimeModeDisplayString(RuntimeMode mode);
QString runtimeStateDisplayString(RuntimeState state);

QString tunDnsHijackModeToString(TunDnsHijackMode mode);
TunDnsHijackMode tunDnsHijackModeFromString(const QString& value);

} // namespace zarya
