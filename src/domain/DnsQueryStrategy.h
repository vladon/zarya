#pragma once

#include <QString>

namespace zarya {

enum class DnsQueryStrategy {
    UseSystemDefault,
    UseIP,
    UseIPv4,
    UseIPv6,
};

QString dnsQueryStrategyToString(DnsQueryStrategy strategy);
DnsQueryStrategy dnsQueryStrategyFromString(const QString& value);
QString dnsQueryStrategyDisplayString(DnsQueryStrategy strategy);
QString dnsQueryStrategyToXrayValue(DnsQueryStrategy strategy);

} // namespace zarya
