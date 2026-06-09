#include "runtime/RuntimeBackendType.h"

#include "i18n/TranslatableEnums.h"

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
    return TranslatableEnums::trRuntimeMode(mode);
}

QString tunDnsHijackModeToString(TunDnsHijackMode mode)
{
    switch (mode) {
    case TunDnsHijackMode::HijackToSingBoxDns:
        return QStringLiteral("hijack-to-singbox-dns");
    case TunDnsHijackMode::Disabled:
        return QStringLiteral("disabled");
    }
    return QStringLiteral("hijack-to-singbox-dns");
}

TunDnsHijackMode tunDnsHijackModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("disabled")) {
        return TunDnsHijackMode::Disabled;
    }
    return TunDnsHijackMode::HijackToSingBoxDns;
}

QString tunPrivilegeModeToString(TunPrivilegeMode mode)
{
    switch (mode) {
    case TunPrivilegeMode::HelperExperimental:
        return QStringLiteral("helper-experimental");
    case TunPrivilegeMode::DirectFromGui:
        return QStringLiteral("direct-gui");
    }
    return QStringLiteral("direct-gui");
}

TunPrivilegeMode tunPrivilegeModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("helper-experimental")) {
        return TunPrivilegeMode::HelperExperimental;
    }
    return TunPrivilegeMode::DirectFromGui;
}

QString runtimeStateDisplayString(RuntimeState state)
{
    return TranslatableEnums::trRuntimeState(state);
}

} // namespace zarya
