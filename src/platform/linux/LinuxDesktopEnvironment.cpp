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
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString currentDesktop = env.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
    const QString session = env.value(QStringLiteral("DESKTOP_SESSION"));
    const QString kdeSession = env.value(QStringLiteral("KDE_FULL_SESSION"));
    const QString gnomeSession = env.value(QStringLiteral("GNOME_DESKTOP_SESSION_ID"));

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
    switch (detect()) {
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
