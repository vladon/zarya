#include "domain/ProtocolType.h"

namespace zarya {

QString protocolTypeToString(ProtocolType type)
{
    switch (type) {
    case ProtocolType::Vless:
        return QStringLiteral("VLESS");
    case ProtocolType::Vmess:
        return QStringLiteral("VMess");
    case ProtocolType::Trojan:
        return QStringLiteral("Trojan");
    case ProtocolType::Shadowsocks:
        return QStringLiteral("Shadowsocks");
    case ProtocolType::Socks:
        return QStringLiteral("SOCKS");
    }
    return QStringLiteral("VLESS");
}

ProtocolType protocolTypeFromString(const QString& value, ProtocolType defaultValue)
{
    if (value.compare(QStringLiteral("VLESS"), Qt::CaseInsensitive) == 0) {
        return ProtocolType::Vless;
    }
    if (value.compare(QStringLiteral("VMess"), Qt::CaseInsensitive) == 0) {
        return ProtocolType::Vmess;
    }
    if (value.compare(QStringLiteral("Trojan"), Qt::CaseInsensitive) == 0) {
        return ProtocolType::Trojan;
    }
    if (value.compare(QStringLiteral("Shadowsocks"), Qt::CaseInsensitive) == 0) {
        return ProtocolType::Shadowsocks;
    }
    if (value.compare(QStringLiteral("SOCKS"), Qt::CaseInsensitive) == 0) {
        return ProtocolType::Socks;
    }
    return defaultValue;
}

} // namespace zarya
