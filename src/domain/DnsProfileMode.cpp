#include "domain/DnsProfileMode.h"

namespace zarya {

QString dnsProfileModeToString(DnsProfileMode mode)
{
    switch (mode) {
    case DnsProfileMode::System:
        return QStringLiteral("system");
    case DnsProfileMode::SecureRemote:
        return QStringLiteral("secure-remote");
    case DnsProfileMode::ChinaDirectGlobalRemote:
        return QStringLiteral("china-direct-global-remote");
    case DnsProfileMode::Custom:
        return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

DnsProfileMode dnsProfileModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("system")) {
        return DnsProfileMode::System;
    }
    if (normalized == QStringLiteral("secure-remote")) {
        return DnsProfileMode::SecureRemote;
    }
    if (normalized == QStringLiteral("china-direct-global-remote")) {
        return DnsProfileMode::ChinaDirectGlobalRemote;
    }
    return DnsProfileMode::Custom;
}

QString dnsProfileModeDisplayString(DnsProfileMode mode)
{
    switch (mode) {
    case DnsProfileMode::System:
        return QStringLiteral("System");
    case DnsProfileMode::SecureRemote:
        return QStringLiteral("Secure Remote");
    case DnsProfileMode::ChinaDirectGlobalRemote:
        return QStringLiteral("China Direct / Global Remote");
    case DnsProfileMode::Custom:
        return QStringLiteral("Custom");
    }
    return QStringLiteral("Custom");
}

namespace DnsBuiltinIds {

QString systemDns()
{
    return QStringLiteral("builtin-dns-system");
}

QString secureRemote()
{
    return QStringLiteral("builtin-dns-secure-remote");
}

QString chinaDirectGlobalRemote()
{
    return QStringLiteral("builtin-dns-china-direct-global-remote");
}

QString customTemplate()
{
    return QStringLiteral("builtin-dns-custom-template");
}

} // namespace DnsBuiltinIds

} // namespace zarya
