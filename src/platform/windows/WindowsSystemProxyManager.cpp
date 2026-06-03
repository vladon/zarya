#include "platform/windows/WindowsSystemProxyManager.h"

#include <QSettings>
#include <windows.h>
#include <wininet.h>

namespace zarya {

namespace {

constexpr auto kInternetSettingsKey =
    R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings)";

QSettings internetSettings()
{
    return QSettings(QString::fromUtf8(kInternetSettingsKey), QSettings::NativeFormat);
}

} // namespace

bool WindowsSystemProxyManager::isSupported() const
{
    return true;
}

QString WindowsSystemProxyManager::backendName() const
{
    return QStringLiteral("Windows WinINet");
}

QString WindowsSystemProxyManager::supportLevel() const
{
    return QStringLiteral("full");
}

QString WindowsSystemProxyManager::limitations() const
{
    return QStringLiteral(
        "Uses WinINet registry settings. Affects applications that use the Windows system proxy.");
}

SystemProxyState WindowsSystemProxyManager::readCurrentState(QString* errorMessage)
{
    Q_UNUSED(errorMessage);

    QSettings settings = internetSettings();
    SystemProxyState state;
    state.proxyEnabled = settings.value(QStringLiteral("ProxyEnable"), 0).toUInt() != 0;
    state.proxyServer = settings.value(QStringLiteral("ProxyServer")).toString();
    state.proxyOverride = settings.value(QStringLiteral("ProxyOverride")).toString();
    state.autoDetect = settings.value(QStringLiteral("AutoDetect"), 0).toUInt() != 0;
    state.autoConfigUrl = settings.value(QStringLiteral("AutoConfigURL")).toString();
    state.platform = QStringLiteral("windows");
    state.backend = backendName();
    state.supportLevel = supportLevel();
    return state;
}

bool WindowsSystemProxyManager::applyHttpProxy(const QString& host, int port,
                                               QString* errorMessage)
{
    if (port < 1 || port > 65535) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid HTTP proxy port: %1").arg(port);
        }
        return false;
    }

    const QString proxyServer =
        QStringLiteral("http=%1:%2;https=%1:%2").arg(host).arg(port);

    QSettings settings = internetSettings();
    settings.setValue(QStringLiteral("ProxyEnable"), 1u);
    settings.setValue(QStringLiteral("ProxyServer"), proxyServer);
    settings.setValue(QStringLiteral("ProxyOverride"), QStringLiteral("<local>"));
    settings.sync();

    if (settings.status() != QSettings::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write Windows proxy registry settings.");
        }
        return false;
    }

    return notifySettingsChanged(errorMessage);
}

bool WindowsSystemProxyManager::restoreState(const SystemProxyState& state,
                                             QString* errorMessage)
{
    QSettings settings = internetSettings();
    settings.setValue(QStringLiteral("ProxyEnable"), state.proxyEnabled ? 1u : 0u);
    settings.setValue(QStringLiteral("ProxyServer"), state.proxyServer);
    settings.setValue(QStringLiteral("ProxyOverride"), state.proxyOverride);
    settings.setValue(QStringLiteral("AutoDetect"), state.autoDetect ? 1u : 0u);

    if (state.autoConfigUrl.isEmpty()) {
        settings.remove(QStringLiteral("AutoConfigURL"));
    } else {
        settings.setValue(QStringLiteral("AutoConfigURL"), state.autoConfigUrl);
    }

    settings.sync();

    if (settings.status() != QSettings::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to restore Windows proxy registry settings.");
        }
        return false;
    }

    return notifySettingsChanged(errorMessage);
}

bool WindowsSystemProxyManager::notifySettingsChanged(QString* errorMessage)
{
    const BOOL settingsChanged =
        InternetSetOptionW(nullptr, INTERNET_OPTION_SETTINGS_CHANGED, nullptr, 0);
    const BOOL refresh = InternetSetOptionW(nullptr, INTERNET_OPTION_REFRESH, nullptr, 0);

    if (!settingsChanged || !refresh) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Windows proxy settings changed notification failed.");
        }
        return false;
    }

    return true;
}

} // namespace zarya
