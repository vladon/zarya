#include "platform/linux/LinuxDesktopEnvironment.h"

#include <QProcessEnvironment>

namespace zarya {

namespace {

bool containsToken(const QString& haystack, const QString& token)
{
    return haystack.contains(token, Qt::CaseInsensitive);
}

} // namespace

LinuxDesktopEnvironment LinuxDesktopEnvironmentDetector::detect()
{
    return detect(QProcessEnvironment::systemEnvironment());
}

LinuxDesktopEnvironment
LinuxDesktopEnvironmentDetector::detect(const QProcessEnvironment& environment)
{
    const QString currentDesktop = environment.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
    const QString session = environment.value(QStringLiteral("DESKTOP_SESSION"));
    const QString kdeSession = environment.value(QStringLiteral("KDE_FULL_SESSION"));
    const QString gnomeSession =
        environment.value(QStringLiteral("GNOME_DESKTOP_SESSION_ID"));

    if (!kdeSession.isEmpty() || containsToken(currentDesktop, QStringLiteral("KDE"))
        || containsToken(session, QStringLiteral("plasma"))
        || containsToken(session, QStringLiteral("kde"))) {
        return LinuxDesktopEnvironment::Kde;
    }

    if (!gnomeSession.isEmpty() || containsToken(currentDesktop, QStringLiteral("GNOME"))
        || containsToken(currentDesktop, QStringLiteral("ubuntu"))
        || containsToken(session, QStringLiteral("gnome"))
        || containsToken(session, QStringLiteral("ubuntu"))) {
        return LinuxDesktopEnvironment::Gnome;
    }

    return LinuxDesktopEnvironment::Unknown;
}

QString LinuxDesktopEnvironmentDetector::detectDisplayName()
{
    return displayName(detect());
}

QString LinuxDesktopEnvironmentDetector::displayName(LinuxDesktopEnvironment environment)
{
    switch (environment) {
    case LinuxDesktopEnvironment::Gnome:
        return QStringLiteral("GNOME");
    case LinuxDesktopEnvironment::Kde:
        return QStringLiteral("KDE/Plasma");
    case LinuxDesktopEnvironment::Unknown:
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

} // namespace zarya
