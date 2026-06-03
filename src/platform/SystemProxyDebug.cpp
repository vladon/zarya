#include "platform/SystemProxyDebug.h"

namespace zarya {

QString formatSystemProxyStateForLog(const SystemProxyState& state)
{
    return QStringLiteral("Proxy enabled: %1\n"
                          "Proxy server: %2\n"
                          "Proxy override: %3\n"
                          "Auto detect: %4\n"
                          "Auto config URL: %5")
        .arg(state.proxyEnabled ? QStringLiteral("true") : QStringLiteral("false"),
             state.proxyServer.isEmpty() ? QStringLiteral("(empty)") : state.proxyServer,
             state.proxyOverride.isEmpty() ? QStringLiteral("(empty)") : state.proxyOverride,
             state.autoDetect ? QStringLiteral("true") : QStringLiteral("false"),
             state.autoConfigUrl.isEmpty() ? QStringLiteral("(empty)") : state.autoConfigUrl);
}

} // namespace zarya
