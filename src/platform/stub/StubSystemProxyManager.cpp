#include "platform/stub/StubSystemProxyManager.h"

namespace zarya {

StubSystemProxyManager::StubSystemProxyManager(QString reason)
    : m_reason(std::move(reason))
{
    if (m_reason.isEmpty()) {
        m_reason = QStringLiteral("System proxy is not supported on this platform.");
    }
}

bool StubSystemProxyManager::isSupported() const
{
    return false;
}

QString StubSystemProxyManager::backendName() const
{
    return QStringLiteral("Unsupported");
}

QString StubSystemProxyManager::supportLevel() const
{
    return QStringLiteral("unsupported");
}

QString StubSystemProxyManager::limitations() const
{
    return m_reason;
}

SystemProxyState StubSystemProxyManager::readCurrentState(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = m_reason;
    }
    SystemProxyState state;
    state.backend = backendName();
    state.supportLevel = supportLevel();
    return state;
}

bool StubSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    (void)host;
    (void)port;
    if (errorMessage) {
        *errorMessage = m_reason;
    }
    return false;
}

bool StubSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    (void)state;
    if (errorMessage) {
        *errorMessage = m_reason;
    }
    return false;
}

} // namespace zarya
