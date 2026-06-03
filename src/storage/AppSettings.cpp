#include "storage/AppSettings.h"

#include "platform/Platform.h"

#include <QSettings>

namespace zarya {

AppSettings& AppSettings::instance()
{
    static AppSettings settings;
    return settings;
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

} // namespace zarya
