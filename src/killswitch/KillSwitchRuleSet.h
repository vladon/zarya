#pragma once

#include "killswitch/KillSwitchMode.h"

#include <QString>
#include <QStringList>

namespace zarya {

struct KillSwitchRuleSet {
    KillSwitchMode mode = KillSwitchMode::TunOnlyExperimental;
    QString tunInterfaceName = QStringLiteral("zarya-tun");
    QString proxyServerHost;
    QStringList proxyServerIpv4;
    QStringList proxyServerIpv6;
    int proxyServerPort = 443;
    bool allowLan = true;
    bool allowLoopback = true;
    bool blockDirectDns = true;
};

} // namespace zarya
