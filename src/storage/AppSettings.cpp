#include "storage/AppSettings.h"

#include "platform/Platform.h"

#include <QSettings>

namespace zarya {

AppSettings& AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

bool AppSettings::defaultAutoEnableSystemProxyOnStart()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

QString AppSettings::xrayExecutablePath() const
{
    return QSettings().value(QStringLiteral("cores/xrayPath")).toString();
}

void AppSettings::setXrayExecutablePath(const QString& path)
{
    QSettings().setValue(QStringLiteral("cores/xrayPath"), path.trimmed());
}

int AppSettings::socksPort() const
{
    const int port = QSettings().value(QStringLiteral("proxy/socksPort"), 10808).toInt();
    return port >= 1 && port <= 65535 ? port : 10808;
}

void AppSettings::setSocksPort(int port)
{
    QSettings().setValue(QStringLiteral("proxy/socksPort"), port);
}

int AppSettings::httpPort() const
{
    const int port = QSettings().value(QStringLiteral("proxy/httpPort"), 10809).toInt();
    return port >= 1 && port <= 65535 ? port : 10809;
}

void AppSettings::setHttpPort(int port)
{
    QSettings().setValue(QStringLiteral("proxy/httpPort"), port);
}

QString AppSettings::resolvedXrayPath() const
{
    const QString configured = xrayExecutablePath().trimmed();
    if (!configured.isEmpty()) {
        return configured;
    }
    return Platform::defaultXrayExecutablePath();
}

bool AppSettings::autoEnableSystemProxyOnStart() const
{
    return QSettings()
        .value(QStringLiteral("proxy/autoEnableSystemProxyOnStart"),
               defaultAutoEnableSystemProxyOnStart())
        .toBool();
}

void AppSettings::setAutoEnableSystemProxyOnStart(bool enabled)
{
    QSettings().setValue(QStringLiteral("proxy/autoEnableSystemProxyOnStart"), enabled);
}

bool AppSettings::restoreProxyOnExit() const
{
    return QSettings().value(QStringLiteral("proxy/restoreProxyOnExit"), true).toBool();
}

void AppSettings::setRestoreProxyOnExit(bool enabled)
{
    QSettings().setValue(QStringLiteral("proxy/restoreProxyOnExit"), enabled);
}

bool AppSettings::confirmBeforeChangingSystemProxy() const
{
    return QSettings().value(QStringLiteral("proxy/confirmBeforeChangingSystemProxy"), false)
        .toBool();
}

void AppSettings::setConfirmBeforeChangingSystemProxy(bool enabled)
{
    QSettings().setValue(QStringLiteral("proxy/confirmBeforeChangingSystemProxy"), enabled);
}

} // namespace zarya
