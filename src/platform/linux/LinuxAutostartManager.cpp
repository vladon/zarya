#include "platform/linux/LinuxAutostartManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace zarya {

QString LinuxAutostartManager::desktopFilePath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
        .filePath(QStringLiteral("autostart/zarya.desktop"));
}

QString LinuxAutostartManager::buildDesktopFile(const QStringList& arguments)
{
    QString executable = QCoreApplication::applicationFilePath();
    QString execLine = QStringLiteral("\"%1\"").arg(executable);
    for (const QString& argument : arguments) {
        execLine += QLatin1Char(' ');
        if (argument.contains(QLatin1Char(' '))) {
            execLine += QStringLiteral("\"%1\"").arg(argument);
        } else {
            execLine += argument;
        }
    }

    return QStringLiteral("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Zarya\n"
                          "Comment=Cross-platform proxy client\n"
                          "Exec=%1\n"
                          "Hidden=false\n"
                          "NoDisplay=false\n"
                          "X-GNOME-Autostart-enabled=true\n"
                          "Terminal=false\n"
                          "Categories=Network;\n")
        .arg(execLine);
}

bool LinuxAutostartManager::isSupported() const
{
    return true;
}

QString LinuxAutostartManager::backendName() const
{
    return QStringLiteral("Linux XDG Autostart");
}

QString LinuxAutostartManager::limitations() const
{
    return QStringLiteral(
        "Uses ~/.config/autostart/zarya.desktop. Works in desktop sessions that honor XDG "
        "autostart. Not for headless/systemd-only sessions.");
}

bool LinuxAutostartManager::isEnabled(QString* errorMessage) const
{
    Q_UNUSED(errorMessage);
    const QString path = desktopFilePath();
    if (!QFile::exists(path)) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    return !QString::fromUtf8(file.readAll()).contains(QStringLiteral("Hidden=true"));
}

bool LinuxAutostartManager::enable(const QStringList& arguments, QString* errorMessage)
{
    const QFileInfo info(desktopFilePath());
    QDir dir = info.dir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QFile file(desktopFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(buildDesktopFile(arguments).toUtf8());
    return true;
}

bool LinuxAutostartManager::disable(QString* errorMessage)
{
    const QString path = desktopFilePath();
    if (!QFile::exists(path)) {
        return true;
    }
    if (QFile::remove(path)) {
        return true;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Failed to remove XDG autostart desktop file.");
    }
    return false;
}

} // namespace zarya
