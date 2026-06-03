#include "app/Application.h"

#include "app/StartupOptions.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"

#include <QSystemTrayIcon>

namespace zarya {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    m_startupOptions = StartupOptionsParser::parse(*this);

    setOrganizationName(QStringLiteral("Zarya"));
    setOrganizationDomain(QStringLiteral("zarya.app"));
    setApplicationName(QStringLiteral("Zarya"));
    setApplicationVersion(PackagingInfo::versionString());

    AppPaths::initialize(m_startupOptions.portable);

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        setQuitOnLastWindowClosed(false);
    }
}

Application* Application::instance()
{
    return qobject_cast<Application*>(QApplication::instance());
}

const StartupOptions& Application::startupOptions() const
{
    return m_startupOptions;
}

} // namespace zarya
