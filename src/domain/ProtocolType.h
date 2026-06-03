#pragma once

#include <QString>

namespace zarya {

enum class ProtocolType {
    Vless,
    Vmess,
    Trojan,
    Shadowsocks,
    Socks,
};

QString protocolTypeToString(ProtocolType type);
ProtocolType protocolTypeFromString(const QString& value,
                                     ProtocolType defaultValue = ProtocolType::Vless);

} // namespace zarya
