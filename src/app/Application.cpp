#include "app/Application.h"

#include <QSystemTrayIcon>

namespace zarya {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setOrganizationName(QStringLiteral("Zarya"));
    setOrganizationDomain(QStringLiteral("zarya.app"));
    setApplicationName(QStringLiteral("Zarya"));
    setApplicationVersion(QStringLiteral("0.8.0"));

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        setQuitOnLastWindowClosed(false);
    }
}

Application* Application::instance()
{
    return qobject_cast<Application*>(QApplication::instance());
}

} // namespace zarya
