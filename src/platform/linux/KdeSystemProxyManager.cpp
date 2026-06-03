#include "platform/linux/KdeSystemProxyManager.h"

namespace zarya {

bool KdeSystemProxyManager::isSupported() const
{
    return false;
}

QString KdeSystemProxyManager::backendName() const
{
    return QStringLiteral("KDE/Plasma");
}

QString KdeSystemProxyManager::supportLevel() const
{
    return QStringLiteral("partial");
}

QString KdeSystemProxyManager::limitations() const
{
    return QStringLiteral(
        "KDE proxy settings may only affect KDE/KIO applications and are not a full system "
        "proxy. Zarya does not modify KDE proxy settings in this milestone.");
}

SystemProxyState KdeSystemProxyManager::readCurrentState(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = limitations();
    }
    SystemProxyState state;
    state.platform = QStringLiteral("linux");
    state.backend = backendName();
    state.supportLevel = supportLevel();
    return state;
}

bool KdeSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    (void)host;
    (void)port;
    if (errorMessage) {
        *errorMessage = limitations();
    }
    return false;
}

bool KdeSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    (void)state;
    if (errorMessage) {
        *errorMessage = limitations();
    }
    return false;
}

} // namespace zarya
