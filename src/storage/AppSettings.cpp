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

QString AppSettings::testUrl() const
{
    return QSettings()
        .value(QStringLiteral("testing/testUrl"), QStringLiteral("https://www.google.com/generate_204"))
        .toString();
}

void AppSettings::setTestUrl(const QString& url)
{
    QSettings().setValue(QStringLiteral("testing/testUrl"), url.trimmed());
}

int AppSettings::tcpTestTimeoutMs() const
{
    const int timeout = QSettings().value(QStringLiteral("testing/tcpTimeoutMs"), 5000).toInt();
    return qBound(1000, timeout, 60000);
}

void AppSettings::setTcpTestTimeoutMs(int timeoutMs)
{
    QSettings().setValue(QStringLiteral("testing/tcpTimeoutMs"), qBound(1000, timeoutMs, 60000));
}

int AppSettings::realDelayTimeoutMs() const
{
    const int timeout = QSettings().value(QStringLiteral("testing/realDelayTimeoutMs"), 10000).toInt();
    return qBound(1000, timeout, 60000);
}

void AppSettings::setRealDelayTimeoutMs(int timeoutMs)
{
    QSettings().setValue(QStringLiteral("testing/realDelayTimeoutMs"),
                         qBound(1000, timeoutMs, 60000));
}

int AppSettings::maxConcurrentTests() const
{
    const int count = QSettings().value(QStringLiteral("testing/maxConcurrentTests"), 3).toInt();
    return qBound(1, count, 10);
}

void AppSettings::setMaxConcurrentTests(int count)
{
    QSettings().setValue(QStringLiteral("testing/maxConcurrentTests"), qBound(1, count, 10));
}

bool AppSettings::skipTcpBeforeRealDelay() const
{
    return QSettings().value(QStringLiteral("testing/skipTcpBeforeRealDelay"), false).toBool();
}

void AppSettings::setSkipTcpBeforeRealDelay(bool skip)
{
    QSettings().setValue(QStringLiteral("testing/skipTcpBeforeRealDelay"), skip);
}

bool AppSettings::defaultMinimizeToTrayOnClose()
{
    return true;
}

bool AppSettings::minimizeToTrayOnClose() const
{
    return QSettings()
        .value(QStringLiteral("desktop/minimizeToTrayOnClose"), defaultMinimizeToTrayOnClose())
        .toBool();
}

void AppSettings::setMinimizeToTrayOnClose(bool enabled)
{
    QSettings().setValue(QStringLiteral("desktop/minimizeToTrayOnClose"), enabled);
}

bool AppSettings::minimizeToTrayOnMinimize() const
{
    return QSettings().value(QStringLiteral("desktop/minimizeToTrayOnMinimize"), false).toBool();
}

void AppSettings::setMinimizeToTrayOnMinimize(bool enabled)
{
    QSettings().setValue(QStringLiteral("desktop/minimizeToTrayOnMinimize"), enabled);
}

bool AppSettings::showTrayNotifications() const
{
    return QSettings().value(QStringLiteral("desktop/showTrayNotifications"), true).toBool();
}

void AppSettings::setShowTrayNotifications(bool enabled)
{
    QSettings().setValue(QStringLiteral("desktop/showTrayNotifications"), enabled);
}

bool AppSettings::confirmExitWhileRunning() const
{
    return QSettings().value(QStringLiteral("desktop/confirmExitWhileRunning"), true).toBool();
}

void AppSettings::setConfirmExitWhileRunning(bool enabled)
{
    QSettings().setValue(QStringLiteral("desktop/confirmExitWhileRunning"), enabled);
}

QString AppSettings::selectedRoutingProfileId() const
{
    return QSettings().value(QStringLiteral("routing/selectedProfileId")).toString();
}

void AppSettings::setSelectedRoutingProfileId(const QString& profileId)
{
    QSettings().setValue(QStringLiteral("routing/selectedProfileId"), profileId.trimmed());
}

} // namespace zarya
