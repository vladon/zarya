#include "platform/stub/StubAutostartManager.h"

namespace zarya {

StubAutostartManager::StubAutostartManager(QString reason)
    : m_reason(std::move(reason))
{
    if (m_reason.isEmpty()) {
        m_reason = QStringLiteral("Autostart is not supported on this platform.");
    }
}

bool StubAutostartManager::isSupported() const
{
    return false;
}

QString StubAutostartManager::backendName() const
{
    return QStringLiteral("Unsupported");
}

QString StubAutostartManager::limitations() const
{
    return m_reason;
}

bool StubAutostartManager::isEnabled(QString* errorMessage) const
{
    Q_UNUSED(errorMessage);
    return false;
}

bool StubAutostartManager::enable(const QStringList& arguments, QString* errorMessage)
{
    Q_UNUSED(arguments);
    if (errorMessage) {
        *errorMessage = m_reason;
    }
    return false;
}

bool StubAutostartManager::disable(QString* errorMessage)
{
    Q_UNUSED(errorMessage);
    return false;
}

} // namespace zarya
