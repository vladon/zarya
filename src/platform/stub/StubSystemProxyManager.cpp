#include "platform/stub/StubSystemProxyManager.h"

namespace zarya {

bool StubSystemProxyManager::isSupported() const
{
    return false;
}

SystemProxyState StubSystemProxyManager::readCurrentState(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("System proxy is not supported on this platform.");
    }
    return {};
}

bool StubSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    (void)host;
    (void)port;
    if (errorMessage) {
        *errorMessage = QStringLiteral("System proxy is not supported on this platform.");
    }
    return false;
}

bool StubSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    (void)state;
    if (errorMessage) {
        *errorMessage = QStringLiteral("System proxy is not supported on this platform.");
    }
    return false;
}

} // namespace zarya
