#include "platform/linux/LinuxSystemProxyManager.h"

#include "platform/linux/GnomeSystemProxyManager.h"
#include "platform/linux/KdeSystemProxyManager.h"
#include "platform/linux/LinuxDesktopEnvironment.h"
#include "platform/stub/StubSystemProxyManager.h"

#include <utility>

namespace zarya {

LinuxSystemProxyManager::LinuxSystemProxyManager()
    : LinuxSystemProxyManager(LinuxDesktopEnvironmentDetector::detect(),
                              defaultPlatformProcessRunner())
{
}

LinuxSystemProxyManager::LinuxSystemProxyManager(LinuxDesktopEnvironment desktop,
                                                 PlatformProcessRunner processRunner)
{
    m_detectedDesktop = LinuxDesktopEnvironmentDetector::displayName(desktop);

    switch (desktop) {
    case LinuxDesktopEnvironment::Gnome: {
        auto gnome = std::make_unique<GnomeSystemProxyManager>(std::move(processRunner));
        if (gnome->isSupported()) {
            m_backend = std::move(gnome);
            break;
        }
        m_backend = std::make_unique<StubSystemProxyManager>(
            QStringLiteral("gsettings is not available for GNOME proxy control."));
        break;
    }
    case LinuxDesktopEnvironment::Kde: {
        auto kde = std::make_unique<KdeSystemProxyManager>(std::move(processRunner));
        if (kde->isSupported()) {
            m_backend = std::move(kde);
            break;
        }
        m_backend = std::make_unique<StubSystemProxyManager>(
            QStringLiteral("KDE config tools are not available for proxy control."));
        break;
    }
    case LinuxDesktopEnvironment::Unknown:
        m_backend = std::make_unique<StubSystemProxyManager>(
            QStringLiteral("Desktop environment is not supported for system proxy yet."));
        break;
    }
}

bool LinuxSystemProxyManager::isSupported() const
{
    return m_backend && m_backend->isSupported();
}

SystemProxyState LinuxSystemProxyManager::readCurrentState(QString* errorMessage)
{
    SystemProxyState state = m_backend->readCurrentState(errorMessage);
    state.platform = QStringLiteral("linux");
    state.backend = backendName();
    state.supportLevel = supportLevel();
    return state;
}

bool LinuxSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    return m_backend->applyHttpProxy(host, port, errorMessage);
}

bool LinuxSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    return m_backend->restoreState(state, errorMessage);
}

QString LinuxSystemProxyManager::backendName() const
{
    if (!m_backend) {
        return QStringLiteral("Linux");
    }
    if (m_backend->supportLevel() == QStringLiteral("partial")) {
        return QStringLiteral("%1 (%2)").arg(m_backend->backendName(), m_detectedDesktop);
    }
    return m_backend->backendName();
}

QString LinuxSystemProxyManager::supportLevel() const
{
    return m_backend ? m_backend->supportLevel() : QStringLiteral("unsupported");
}

QString LinuxSystemProxyManager::limitations() const
{
    return m_backend ? m_backend->limitations() : QString();
}

QString LinuxSystemProxyManager::detectedDesktopName() const
{
    return m_detectedDesktop;
}

} // namespace zarya
