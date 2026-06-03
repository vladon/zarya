#include "domain/DnsQueryStrategy.h"

namespace zarya {

QString dnsQueryStrategyToString(DnsQueryStrategy strategy)
{
    switch (strategy) {
    case DnsQueryStrategy::UseSystemDefault:
        return QStringLiteral("system-default");
    case DnsQueryStrategy::UseIP:
        return QStringLiteral("use-ip");
    case DnsQueryStrategy::UseIPv4:
        return QStringLiteral("use-ipv4");
    case DnsQueryStrategy::UseIPv6:
        return QStringLiteral("use-ipv6");
    }
    return QStringLiteral("system-default");
}

DnsQueryStrategy dnsQueryStrategyFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("use-ip")) {
        return DnsQueryStrategy::UseIP;
    }
    if (normalized == QStringLiteral("use-ipv4")) {
        return DnsQueryStrategy::UseIPv4;
    }
    if (normalized == QStringLiteral("use-ipv6")) {
        return DnsQueryStrategy::UseIPv6;
    }
    return DnsQueryStrategy::UseSystemDefault;
}

QString dnsQueryStrategyDisplayString(DnsQueryStrategy strategy)
{
    switch (strategy) {
    case DnsQueryStrategy::UseSystemDefault:
        return QStringLiteral("System default");
    case DnsQueryStrategy::UseIP:
        return QStringLiteral("UseIP");
    case DnsQueryStrategy::UseIPv4:
        return QStringLiteral("UseIPv4");
    case DnsQueryStrategy::UseIPv6:
        return QStringLiteral("UseIPv6");
    }
    return QStringLiteral("System default");
}

QString dnsQueryStrategyToXrayValue(DnsQueryStrategy strategy)
{
    switch (strategy) {
    case DnsQueryStrategy::UseIP:
        return QStringLiteral("UseIP");
    case DnsQueryStrategy::UseIPv4:
        return QStringLiteral("UseIPv4");
    case DnsQueryStrategy::UseIPv6:
        return QStringLiteral("UseIPv6");
    case DnsQueryStrategy::UseSystemDefault:
        return {};
    }
    return {};
}

} // namespace zarya
