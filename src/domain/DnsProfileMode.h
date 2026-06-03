#pragma once

#include <QString>

namespace zarya {

enum class DnsProfileMode {
    System,
    SecureRemote,
    ChinaDirectGlobalRemote,
    Custom,
};

QString dnsProfileModeToString(DnsProfileMode mode);
DnsProfileMode dnsProfileModeFromString(const QString& value);
QString dnsProfileModeDisplayString(DnsProfileMode mode);

namespace DnsBuiltinIds {
QString systemDns();
QString secureRemote();
QString chinaDirectGlobalRemote();
QString customTemplate();
} // namespace DnsBuiltinIds

} // namespace zarya
