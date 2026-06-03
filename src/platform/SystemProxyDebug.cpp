#include "platform/SystemProxyDebug.h"

namespace zarya {

QString formatSystemProxyStateForLog(const SystemProxyState& state)
{
    QString text = QStringLiteral("Platform: %1\n"
                                  "Backend: %2\n"
                                  "Support level: %3\n"
                                  "Proxy enabled: %4\n"
                                  "Proxy server: %5\n"
                                  "Proxy override: %6\n"
                                  "Auto detect: %7\n"
                                  "Auto config URL: %8")
                       .arg(state.platform.isEmpty() ? QStringLiteral("(n/a)") : state.platform,
                            state.backend.isEmpty() ? QStringLiteral("(n/a)") : state.backend,
                            state.supportLevel.isEmpty() ? QStringLiteral("(n/a)")
                                                         : state.supportLevel,
                            state.proxyEnabled ? QStringLiteral("true") : QStringLiteral("false"),
                            state.proxyServer.isEmpty() ? QStringLiteral("(empty)")
                                                        : state.proxyServer,
                            state.proxyOverride.isEmpty() ? QStringLiteral("(empty)")
                                                          : state.proxyOverride,
                            state.autoDetect ? QStringLiteral("true") : QStringLiteral("false"),
                            state.autoConfigUrl.isEmpty() ? QStringLiteral("(empty)")
                                                          : state.autoConfigUrl);

    if (!state.activeNetworkService.isEmpty()) {
        text += QStringLiteral("\nActive network service: %1").arg(state.activeNetworkService);
    }
    if (!state.affectedNetworkServices.isEmpty()) {
        text += QStringLiteral("\nAffected services: %1")
                    .arg(state.affectedNetworkServices.join(QStringLiteral(", ")));
    }
    return text;
}

} // namespace zarya
