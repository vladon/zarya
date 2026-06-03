#include "app/Application.h"

namespace zarya {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setOrganizationName(QStringLiteral("Zarya"));
    setOrganizationDomain(QStringLiteral("zarya.app"));
    setApplicationName(QStringLiteral("Zarya"));
    setApplicationVersion(QStringLiteral("0.6.0"));
}

Application* Application::instance()
{
    return qobject_cast<Application*>(QApplication::instance());
}

} // namespace zarya
