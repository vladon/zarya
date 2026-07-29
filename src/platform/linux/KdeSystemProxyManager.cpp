#include "platform/linux/KdeSystemProxyManager.h"

#include <QUrl>

#include <array>
#include <utility>

namespace zarya {

namespace {

constexpr auto kConfigFile = "kioslaverc";
constexpr auto kConfigGroup = "Proxy Settings";
constexpr auto kMissingValue = "__zarya_kde_proxy_key_missing_5e83f262__";
constexpr int kCommandTimeoutMs = 5000;

const std::array<QString, 6>& managedKeys()
{
    static const std::array<QString, 6> keys = {
        QStringLiteral("httpProxy"),
        QStringLiteral("httpsProxy"),
        QStringLiteral("NoProxyFor"),
        QStringLiteral("Proxy Config Script"),
        QStringLiteral("ReversedException"),
        QStringLiteral("ProxyType"),
    };
    return keys;
}

const std::array<QString, 5>& appliedKeys()
{
    static const std::array<QString, 5> keys = {
        QStringLiteral("httpProxy"),
        QStringLiteral("httpsProxy"),
        QStringLiteral("NoProxyFor"),
        QStringLiteral("ReversedException"),
        QStringLiteral("ProxyType"),
    };
    return keys;
}

QString withoutTrailingLineBreaks(QString value)
{
    while (value.endsWith(QLatin1Char('\n')) || value.endsWith(QLatin1Char('\r'))) {
        value.chop(1);
    }
    return value;
}

QVariantMap snapshotEntry(bool present, const QString& value)
{
    return {
        {QStringLiteral("present"), present},
        {QStringLiteral("value"), value},
    };
}

} // namespace

KdeSystemProxyManager::KdeSystemProxyManager(PlatformProcessRunner processRunner)
    : m_processRunner(processRunner ? std::move(processRunner)
                                    : defaultPlatformProcessRunner())
{
}

KdeSystemProxyManager::ConfigTools KdeSystemProxyManager::detectConfigTools() const
{
    const std::array<ConfigTools, 2> candidates = {
        ConfigTools{QStringLiteral("kreadconfig6"), QStringLiteral("kwriteconfig6")},
        ConfigTools{QStringLiteral("kreadconfig5"), QStringLiteral("kwriteconfig5")},
    };
    for (const ConfigTools& candidate : candidates) {
        const ProcessResult read =
            m_processRunner(candidate.read, {QStringLiteral("--help")}, kCommandTimeoutMs);
        const ProcessResult write =
            m_processRunner(candidate.write, {QStringLiteral("--help")}, kCommandTimeoutMs);
        if (read.success && write.success) {
            return candidate;
        }
    }
    return {};
}

bool KdeSystemProxyManager::isSupported() const
{
    if (!detectConfigTools().isValid()) {
        return false;
    }
    const ProcessResult sessionBus = m_processRunner(
        QStringLiteral("dbus-send"),
        {QStringLiteral("--session"), QStringLiteral("--print-reply=literal"),
         QStringLiteral("--dest=org.freedesktop.DBus"), QStringLiteral("/"),
         QStringLiteral("org.freedesktop.DBus.ListNames")},
        kCommandTimeoutMs);
    return sessionBus.success;
}

QString KdeSystemProxyManager::backendName() const
{
    return QStringLiteral("KDE/Plasma kioslaverc");
}

QString KdeSystemProxyManager::supportLevel() const
{
    return isSupported() ? QStringLiteral("full") : QStringLiteral("unsupported");
}

QString KdeSystemProxyManager::limitations() const
{
    return QStringLiteral(
        "Uses KDE kioslaverc proxy settings. Affects KDE/KIO and applications that respect "
        "desktop proxy settings; CLI tools may need http_proxy environment variables.");
}

bool KdeSystemProxyManager::readConfigValue(const ConfigTools& tools, const QString& key,
                                            bool* present, QString* value,
                                            QString* errorMessage) const
{
    const ProcessResult result = m_processRunner(
        tools.read,
        {QStringLiteral("--file"), QString::fromUtf8(kConfigFile),
         QStringLiteral("--group"), QString::fromUtf8(kConfigGroup),
         QStringLiteral("--key"), key,
         QStringLiteral("--default"), QString::fromUtf8(kMissingValue)},
        kCommandTimeoutMs);
    if (!result.success) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("%1 failed while reading KDE proxy key %2: %3")
                    .arg(tools.read, key, result.errorMessage);
        }
        return false;
    }

    const QString rawValue = withoutTrailingLineBreaks(result.standardOutput);
    *present = rawValue != QString::fromUtf8(kMissingValue);
    *value = *present ? rawValue : QString();
    return true;
}

bool KdeSystemProxyManager::writeConfigValue(const ConfigTools& tools, const QString& key,
                                             bool present, const QString& value,
                                             QString* errorMessage) const
{
    QStringList arguments = {
        QStringLiteral("--file"), QString::fromUtf8(kConfigFile),
        QStringLiteral("--group"), QString::fromUtf8(kConfigGroup),
        QStringLiteral("--key"), key,
    };
    arguments.append(present ? value : QStringLiteral("--delete"));

    const ProcessResult result =
        m_processRunner(tools.write, arguments, kCommandTimeoutMs);
    if (!result.success) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("%1 failed while writing KDE proxy key %2: %3")
                    .arg(tools.write, key, result.errorMessage);
        }
        return false;
    }
    return true;
}

bool KdeSystemProxyManager::reloadKio(QString* errorMessage) const
{
    const ProcessResult result = m_processRunner(
        QStringLiteral("dbus-send"),
        {QStringLiteral("--session"), QStringLiteral("--type=signal"),
         QStringLiteral("/KIO/Scheduler"),
         QStringLiteral("org.kde.KIO.Scheduler.reparseSlaveConfiguration"),
         QStringLiteral("string:")},
        kCommandTimeoutMs);
    if (!result.success) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("Failed to notify running KDE/KIO applications: %1")
                    .arg(result.errorMessage);
        }
        return false;
    }
    return true;
}

SystemProxyState KdeSystemProxyManager::readCurrentState(QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    SystemProxyState state;
    state.platform = QStringLiteral("linux");
    state.backend = backendName();
    state.supportLevel = supportLevel();

    const ConfigTools tools = detectConfigTools();
    if (!tools.isValid()) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("KDE config tools are unavailable (tried kreadconfig6/"
                               "kwriteconfig6 and kreadconfig5/kwriteconfig5).");
        }
        return state;
    }

    QVariantMap raw;
    for (const QString& key : managedKeys()) {
        bool present = false;
        QString value;
        if (!readConfigValue(tools, key, &present, &value, errorMessage)) {
            return state;
        }
        raw.insert(key, snapshotEntry(present, value));
    }
    state.rawValues.insert(QStringLiteral("kioslaverc"), raw);

    const QVariantMap proxyType = raw.value(QStringLiteral("ProxyType")).toMap();
    state.proxyEnabled =
        proxyType.value(QStringLiteral("present")).toBool()
        && proxyType.value(QStringLiteral("value")).toString() == QStringLiteral("1");
    const QString httpProxy =
        raw.value(QStringLiteral("httpProxy")).toMap().value(QStringLiteral("value")).toString();
    const QUrl proxyUrl(httpProxy);
    if (proxyUrl.isValid() && !proxyUrl.host().isEmpty() && proxyUrl.port() > 0) {
        state.proxyServer =
            QStringLiteral("%1:%2").arg(proxyUrl.host()).arg(proxyUrl.port());
    }
    return state;
}

bool KdeSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (host.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("HTTP proxy host must not be empty.");
        }
        return false;
    }
    if (port < 1 || port > 65535) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid HTTP proxy port: %1").arg(port);
        }
        return false;
    }

    QString snapshotError;
    const SystemProxyState previous = readCurrentState(&snapshotError);
    if (!snapshotError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = snapshotError;
        }
        return false;
    }

    const ConfigTools tools = detectConfigTools();
    const QString endpoint =
        QUrl(QStringLiteral("http://%1:%2").arg(host).arg(port)).toString();
    const QVariantMap desired = {
        {QStringLiteral("httpProxy"), snapshotEntry(true, endpoint)},
        {QStringLiteral("httpsProxy"), snapshotEntry(true, endpoint)},
        {QStringLiteral("NoProxyFor"),
         snapshotEntry(true, QStringLiteral("localhost,127.0.0.1,::1"))},
        {QStringLiteral("ReversedException"), snapshotEntry(true, QStringLiteral("false"))},
        {QStringLiteral("ProxyType"), snapshotEntry(true, QStringLiteral("1"))},
    };

    QString applyError;
    for (const QString& key : appliedKeys()) {
        const QVariantMap entry = desired.value(key).toMap();
        if (!writeConfigValue(tools, key, true,
                              entry.value(QStringLiteral("value")).toString(), &applyError)) {
            break;
        }
    }
    if (applyError.isEmpty()) {
        reloadKio(&applyError);
    }
    if (applyError.isEmpty()) {
        return true;
    }

    QString rollbackError;
    const bool rolledBack =
        writeSnapshot(previous, &rollbackError) && reloadKio(&rollbackError);
    if (errorMessage) {
        *errorMessage = rolledBack
                            ? QStringLiteral("%1 Previous KDE proxy settings were restored.")
                                  .arg(applyError)
                            : QStringLiteral("%1 Rollback failed: %2")
                                  .arg(applyError, rollbackError);
    }
    return false;
}

bool KdeSystemProxyManager::writeSnapshot(const SystemProxyState& state,
                                          QString* errorMessage) const
{
    const QVariantMap raw = state.rawValues.value(QStringLiteral("kioslaverc")).toMap();
    if (raw.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Missing KDE kioslaverc snapshot for restore.");
        }
        return false;
    }

    const ConfigTools tools = detectConfigTools();
    if (!tools.isValid()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("KDE config tools are unavailable during restore.");
        }
        return false;
    }

    for (const QString& key : managedKeys()) {
        const QVariantMap entry = raw.value(key).toMap();
        if (entry.isEmpty() || !entry.contains(QStringLiteral("present"))
            || !entry.contains(QStringLiteral("value"))) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("KDE snapshot is missing proxy key metadata for %1.").arg(key);
            }
            return false;
        }
        if (!writeConfigValue(tools, key,
                              entry.value(QStringLiteral("present")).toBool(),
                              entry.value(QStringLiteral("value")).toString(), errorMessage)) {
            return false;
        }
    }
    return true;
}

bool KdeSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    QString currentError;
    const SystemProxyState current = readCurrentState(&currentError);
    if (!currentError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = currentError;
        }
        return false;
    }

    QString restoreError;
    if (writeSnapshot(state, &restoreError) && reloadKio(&restoreError)) {
        return true;
    }

    QString rollbackError;
    const bool rolledBack =
        writeSnapshot(current, &rollbackError) && reloadKio(&rollbackError);
    if (errorMessage) {
        *errorMessage = rolledBack
                            ? QStringLiteral("%1 Current KDE proxy settings were preserved.")
                                  .arg(restoreError)
                            : QStringLiteral("%1 Rollback failed: %2")
                                  .arg(restoreError, rollbackError);
    }
    return false;
}

} // namespace zarya
