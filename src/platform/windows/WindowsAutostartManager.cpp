#include "platform/windows/WindowsAutostartManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace zarya {

namespace {

constexpr auto kRunKeyPath =
    R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)";

} // namespace

QString WindowsAutostartManager::runKeyPath()
{
    return QString::fromUtf8(kRunKeyPath);
}

QString WindowsAutostartManager::valueName()
{
    return QStringLiteral("Zarya");
}

QString WindowsAutostartManager::buildCommandLine(const QStringList& arguments)
{
    QString executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    if (!executable.startsWith(QLatin1Char('"'))) {
        executable = QStringLiteral("\"%1\"").arg(executable);
    }
    QString command = executable;
    for (const QString& argument : arguments) {
        command += QLatin1Char(' ');
        if (argument.contains(QLatin1Char(' '))) {
            command += QStringLiteral("\"%1\"").arg(argument);
        } else {
            command += argument;
        }
    }
    return command;
}

bool WindowsAutostartManager::isSupported() const
{
    return true;
}

QString WindowsAutostartManager::backendName() const
{
    return QStringLiteral("Windows Run key");
}

QString WindowsAutostartManager::limitations() const
{
    return QStringLiteral("Uses HKCU Run registry entry for the current user.");
}

bool WindowsAutostartManager::isEnabled(QString* errorMessage) const
{
    Q_UNUSED(errorMessage);
    QSettings settings(runKeyPath(), QSettings::NativeFormat);
    return settings.contains(valueName());
}

bool WindowsAutostartManager::enable(const QStringList& arguments, QString* errorMessage)
{
    QSettings settings(runKeyPath(), QSettings::NativeFormat);
    settings.setValue(valueName(), buildCommandLine(arguments));
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write Windows Run key autostart entry.");
        }
        return false;
    }
    return true;
}

bool WindowsAutostartManager::disable(QString* errorMessage)
{
    QSettings settings(runKeyPath(), QSettings::NativeFormat);
    settings.remove(valueName());
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to remove Windows Run key autostart entry.");
        }
        return false;
    }
    return true;
}

} // namespace zarya
