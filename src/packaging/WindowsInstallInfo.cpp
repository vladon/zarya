#include "packaging/WindowsInstallInfo.h"

#if defined(Q_OS_WIN)

#include <QProcess>
#include <QRegularExpression>
#include <QSettings>

namespace zarya {

namespace {

QString registryString(const QString& name)
{
    QSettings settings(QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\Zarya"),
                       QSettings::NativeFormat);
    return settings.value(name).toString().trimmed();
}

QString scQueryState()
{
    QProcess process;
    process.start(QStringLiteral("sc"),
                  {QStringLiteral("query"), QStringLiteral("ZaryaHelper")});
    if (!process.waitForFinished(5000)) {
        process.kill();
        return {};
    }
    if (process.exitCode() != 0) {
        return QStringLiteral("not_installed");
    }
    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    for (const QString& line : output.split(QRegularExpression(QStringLiteral("[\\r\\n]+")))) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("STATE"), Qt::CaseInsensitive)) {
            if (trimmed.contains(QStringLiteral("RUNNING"), Qt::CaseInsensitive)) {
                return QStringLiteral("running");
            }
            if (trimmed.contains(QStringLiteral("STOPPED"), Qt::CaseInsensitive)) {
                return QStringLiteral("stopped");
            }
            return QStringLiteral("installed");
        }
    }
    return QStringLiteral("installed");
}

} // namespace

bool WindowsInstallInfo::isAvailable()
{
    return true;
}

QString WindowsInstallInfo::installerType()
{
    const QString type = registryString(QStringLiteral("InstallerType"));
    return type.isEmpty() ? QStringLiteral("unknown") : type;
}

QString WindowsInstallInfo::installDirectory()
{
    return registryString(QStringLiteral("InstallDir"));
}

QString WindowsInstallInfo::registryVersion()
{
    return registryString(QStringLiteral("Version"));
}

bool WindowsInstallInfo::helperServiceInstalled()
{
    return scQueryState() != QStringLiteral("not_installed");
}

QString WindowsInstallInfo::helperServiceState()
{
    return scQueryState();
}

} // namespace zarya

#else

namespace zarya {

bool WindowsInstallInfo::isAvailable() { return false; }
QString WindowsInstallInfo::installerType() { return {}; }
QString WindowsInstallInfo::installDirectory() { return {}; }
QString WindowsInstallInfo::registryVersion() { return {}; }
bool WindowsInstallInfo::helperServiceInstalled() { return false; }
QString WindowsInstallInfo::helperServiceState() { return {}; }

} // namespace zarya

#endif
