#include "platform/linux/GnomeSystemProxyManager.h"

#include "platform/PlatformProcessUtils.h"

namespace zarya {

namespace {

constexpr auto kProxySchema = "org.gnome.system.proxy";
constexpr auto kHttpSchema = "org.gnome.system.proxy.http";
constexpr auto kHttpsSchema = "org.gnome.system.proxy.https";
constexpr auto kSocksSchema = "org.gnome.system.proxy.socks";

QString stripQuotes(const QString& raw)
{
    QString value = raw.trimmed();
    if (value.size() >= 2 && value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))) {
        return value.mid(1, value.size() - 2);
    }
    return value;
}

} // namespace

bool GnomeSystemProxyManager::isSupported() const
{
    const ProcessResult result =
        runProcess(QStringLiteral("gsettings"), {QStringLiteral("help")});
    return result.success;
}

QString GnomeSystemProxyManager::backendName() const
{
    return QStringLiteral("GNOME gsettings");
}

QString GnomeSystemProxyManager::supportLevel() const
{
    return isSupported() ? QStringLiteral("full") : QStringLiteral("unsupported");
}

QString GnomeSystemProxyManager::limitations() const
{
    return QStringLiteral(
        "Uses gsettings (org.gnome.system.proxy). Affects GNOME/GTK applications that respect "
        "desktop proxy settings. CLI tools may need http_proxy environment variables.");
}

QString GnomeSystemProxyManager::readGsettingsValue(const QString& schemaKey,
                                                    QString* errorMessage) const
{
    const ProcessResult result =
        runProcess(QStringLiteral("gsettings"), {QStringLiteral("get"), schemaKey});
    if (!result.success) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("gsettings get %1 failed: %2").arg(schemaKey, result.errorMessage);
        }
        return {};
    }
    return result.standardOutput.trimmed();
}

bool GnomeSystemProxyManager::setGsettingsValue(const QString& schemaKey, const QString& rawValue,
                                                QString* errorMessage) const
{
    const ProcessResult result = runProcess(
        QStringLiteral("gsettings"), {QStringLiteral("set"), schemaKey, rawValue});
    if (!result.success) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("gsettings set %1 %2 failed: %3")
                    .arg(schemaKey, rawValue, result.errorMessage);
        }
        return false;
    }
    return true;
}

bool GnomeSystemProxyManager::setGsettingsValue(const QString& schemaKey, int value,
                                                QString* errorMessage) const
{
    const ProcessResult result = runProcess(
        QStringLiteral("gsettings"), {QStringLiteral("set"), schemaKey, QString::number(value)});
    if (!result.success) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("gsettings set %1 %2 failed: %3")
                    .arg(schemaKey, QString::number(value), result.errorMessage);
        }
        return false;
    }
    return true;
}

SystemProxyState GnomeSystemProxyManager::readCurrentState(QString* errorMessage)
{
    SystemProxyState state;
    state.platform = QStringLiteral("linux");
    state.backend = backendName();
    state.supportLevel = supportLevel();

    QVariantMap raw;
    const auto storeRaw = [&](const QString& key, const QString& schemaKey) {
        const QString value = readGsettingsValue(schemaKey, errorMessage);
        if (errorMessage && !errorMessage->isEmpty()) {
            return false;
        }
        raw.insert(key, value);
        return true;
    };

    if (!storeRaw(QStringLiteral("mode"), QString::fromUtf8(kProxySchema) + QStringLiteral(".mode"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("ignoreHosts"),
                  QString::fromUtf8(kProxySchema) + QStringLiteral(".ignore-hosts"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("useSameProxy"),
                  QString::fromUtf8(kProxySchema) + QStringLiteral(".use-same-proxy"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("autoconfigUrl"),
                  QString::fromUtf8(kProxySchema) + QStringLiteral(".autoconfig-url"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("httpHost"), QString::fromUtf8(kHttpSchema) + QStringLiteral(".host"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("httpPort"), QString::fromUtf8(kHttpSchema) + QStringLiteral(".port"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("httpsHost"),
                  QString::fromUtf8(kHttpsSchema) + QStringLiteral(".host"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("httpsPort"),
                  QString::fromUtf8(kHttpsSchema) + QStringLiteral(".port"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("socksHost"),
                  QString::fromUtf8(kSocksSchema) + QStringLiteral(".host"))) {
        return state;
    }
    if (!storeRaw(QStringLiteral("socksPort"),
                  QString::fromUtf8(kSocksSchema) + QStringLiteral(".port"))) {
        return state;
    }

    state.rawValues.insert(QStringLiteral("gsettings"), raw);

    const QString mode = stripQuotes(raw.value(QStringLiteral("mode")).toString());
    state.proxyEnabled = mode == QStringLiteral("manual");
    const QString httpHost = stripQuotes(raw.value(QStringLiteral("httpHost")).toString());
    const int httpPort = raw.value(QStringLiteral("httpPort")).toString().toInt();
    if (!httpHost.isEmpty() && httpPort > 0) {
        state.proxyServer = QStringLiteral("%1:%2").arg(httpHost).arg(httpPort);
    }
    return state;
}

bool GnomeSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    if (port < 1 || port > 65535) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid HTTP proxy port: %1").arg(port);
        }
        return false;
    }

    if (!setGsettingsValue(QString::fromUtf8(kProxySchema) + QStringLiteral(".mode"),
                         QStringLiteral("'manual'"), errorMessage)) {
        return false;
    }
    if (!setGsettingsValue(QString::fromUtf8(kProxySchema) + QStringLiteral(".use-same-proxy"),
                         QStringLiteral("false"), errorMessage)) {
        return false;
    }
    if (!setGsettingsValue(QString::fromUtf8(kHttpSchema) + QStringLiteral(".host"),
                         QStringLiteral("'%1'").arg(host), errorMessage)) {
        return false;
    }
    if (!setGsettingsValue(QString::fromUtf8(kHttpSchema) + QStringLiteral(".port"), port,
                           errorMessage)) {
        return false;
    }
    if (!setGsettingsValue(QString::fromUtf8(kHttpsSchema) + QStringLiteral(".host"),
                         QStringLiteral("'%1'").arg(host), errorMessage)) {
        return false;
    }
    if (!setGsettingsValue(QString::fromUtf8(kHttpsSchema) + QStringLiteral(".port"), port,
                           errorMessage)) {
        return false;
    }
    return setGsettingsValue(
        QString::fromUtf8(kProxySchema) + QStringLiteral(".ignore-hosts"),
        QStringLiteral("['localhost', '127.0.0.0/8', '::1']"), errorMessage);
}

bool GnomeSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    const QVariantMap raw = state.rawValues.value(QStringLiteral("gsettings")).toMap();
    if (raw.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Missing GNOME gsettings snapshot for restore.");
        }
        return false;
    }

    struct KeyMapping {
        QString key;
        QString schema;
    };

    const KeyMapping mappings[] = {
        {QStringLiteral("mode"), QStringLiteral(".mode")},
        {QStringLiteral("ignoreHosts"), QStringLiteral(".ignore-hosts")},
        {QStringLiteral("useSameProxy"), QStringLiteral(".use-same-proxy")},
        {QStringLiteral("autoconfigUrl"), QStringLiteral(".autoconfig-url")},
    };

    for (const KeyMapping& mapping : mappings) {
        if (!raw.contains(mapping.key)) {
            continue;
        }
        const QString schema = QString::fromUtf8(kProxySchema) + mapping.schema;
        if (!setGsettingsValue(schema, raw.value(mapping.key).toString(), errorMessage)) {
            return false;
        }
    }

    const auto restoreEndpoint = [&](const QString& hostKey, const QString& portKey,
                                     const char* schemaBase) -> bool {
        if (raw.contains(hostKey)) {
            const QString schema = QString::fromUtf8(schemaBase) + QStringLiteral(".host");
            if (!setGsettingsValue(schema, raw.value(hostKey).toString(), errorMessage)) {
                return false;
            }
        }
        if (raw.contains(portKey)) {
            const QString schema = QString::fromUtf8(schemaBase) + QStringLiteral(".port");
            if (!setGsettingsValue(schema, raw.value(portKey).toString().toInt(), errorMessage)) {
                return false;
            }
        }
        return true;
    };

    if (!restoreEndpoint(QStringLiteral("httpHost"), QStringLiteral("httpPort"), kHttpSchema)) {
        return false;
    }
    if (!restoreEndpoint(QStringLiteral("httpsHost"), QStringLiteral("httpsPort"), kHttpsSchema)) {
        return false;
    }
    if (!restoreEndpoint(QStringLiteral("socksHost"), QStringLiteral("socksPort"), kSocksSchema)) {
        return false;
    }

    return true;
}

} // namespace zarya
