#include "killswitch/KillSwitchManager.h"

#include "killswitch/KillSwitchMarker.h"
#include "killswitch/KillSwitchMode.h"

#if defined(Q_OS_LINUX)
#include "killswitch/linux/LinuxNftKillSwitchManager.h"
#elif defined(Q_OS_WIN)
#include "killswitch/windows/WindowsKillSwitchManager.h"
#elif defined(Q_OS_MACOS)
#include "killswitch/macos/MacKillSwitchManager.h"
#else
#include "killswitch/stub/StubKillSwitchManager.h"
#endif

#include <QJsonArray>
#include <QJsonObject>

namespace zarya {

KillSwitchManager::KillSwitchManager(QObject* parent)
    : QObject(parent)
    , m_backend(createBackend())
{
    m_state.backend = m_backend ? m_backend->backendId() : QStringLiteral("unknown");
    m_state.status = KillSwitchStatus::Disabled;
}

std::unique_ptr<IKillSwitchBackend> KillSwitchManager::createBackend()
{
#if defined(Q_OS_LINUX)
    return std::make_unique<LinuxNftKillSwitchManager>();
#elif defined(Q_OS_WIN)
    return std::make_unique<WindowsKillSwitchManager>();
#elif defined(Q_OS_MACOS)
    return std::make_unique<MacKillSwitchManager>();
#else
    return std::make_unique<StubKillSwitchManager>(QStringLiteral("unknown"));
#endif
}

KillSwitchState KillSwitchManager::state() const
{
    return m_state;
}

void KillSwitchManager::refreshStartupState(bool privileged)
{
    m_state.recoveryMarkerPresent = KillSwitchMarker::exists();
    if (m_state.recoveryMarkerPresent) {
        KillSwitchMarkerData marker;
        if (KillSwitchMarker::read(&marker)) {
            m_state.backend = marker.backend;
            m_state.enabledAt = QDateTime::fromString(marker.enabledAt, Qt::ISODate);
        }
        m_state.status = KillSwitchStatus::NeedsRecovery;
        m_state.privileged = privileged;
        emit stateChanged(m_state);
        emit logLine(QStringLiteral("helper: kill switch recovery marker detected"));
        return;
    }
    m_state.status = KillSwitchStatus::Disabled;
    m_state.privileged = privileged;
    emit stateChanged(m_state);
}

KillSwitchState KillSwitchManager::checkSupport(bool privileged) const
{
    if (!m_backend) {
        KillSwitchState state;
        state.status = KillSwitchStatus::Unsupported;
        state.lastError = QStringLiteral("Kill switch backend unavailable.");
        return state;
    }
    KillSwitchState state = m_backend->checkSupport(privileged);
    state.privileged = privileged;
    return state;
}

bool KillSwitchManager::enable(const KillSwitchRuleSet& rules, bool privileged, QString* errorMessage)
{
    if (!m_backend) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Kill switch backend unavailable.");
        }
        return false;
    }

    KillSwitchState support = m_backend->checkSupport(privileged);
    if (!support.supported) {
        if (errorMessage) {
            *errorMessage = support.lastError;
        }
        setState(support);
        return false;
    }
    if (!privileged) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot enable kill switch: helper is not privileged.");
        }
        KillSwitchState failed = m_state;
        failed.status = KillSwitchStatus::Failed;
        failed.lastError = *errorMessage;
        failed.privileged = false;
        setState(failed);
        return false;
    }

    KillSwitchState enabling = m_state;
    enabling.status = KillSwitchStatus::Enabling;
    setState(enabling);

    QString error;
    if (!m_backend->enable(rules, &error)) {
        KillSwitchState failed = m_state;
        failed.status = KillSwitchStatus::Failed;
        failed.lastError = error;
        setState(failed);
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    KillSwitchMarkerData marker;
    marker.backend = m_backend->backendId();
    marker.enabledAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    marker.rulesetName = QStringLiteral("zarya");
    marker.tunInterfaceName = rules.tunInterfaceName;
    if (!KillSwitchMarker::write(marker, &error)) {
        m_backend->disable(nullptr);
        if (errorMessage) {
            *errorMessage = error;
        }
        KillSwitchState failed = m_state;
        failed.status = KillSwitchStatus::Failed;
        failed.lastError = error;
        setState(failed);
        return false;
    }

    KillSwitchState enabled = m_state;
    enabled.status = KillSwitchStatus::Enabled;
    enabled.mode = rules.mode;
    enabled.backend = m_backend->backendId();
    enabled.enabledAt = QDateTime::currentDateTimeUtc();
    enabled.recoveryMarkerPresent = true;
    enabled.lastError.clear();
    enabled.activeRules = {QStringLiteral("table inet zarya")};
    setState(enabled);
    emit logLine(QStringLiteral("helper: kill switch enabled (%1)").arg(enabled.backend));
    return true;
}

bool KillSwitchManager::disable(QString* errorMessage)
{
    if (!m_backend) {
        return true;
    }

    KillSwitchState disabling = m_state;
    disabling.status = KillSwitchStatus::Disabling;
    setState(disabling);

    QString error;
    if (m_state.status == KillSwitchStatus::Enabled || KillSwitchMarker::exists()) {
        if (!m_backend->disable(&error)) {
            KillSwitchState failed = m_state;
            failed.status = KillSwitchStatus::Failed;
            failed.lastError = error;
            setState(failed);
            if (errorMessage) {
                *errorMessage = error;
            }
            return false;
        }
    }

    if (!KillSwitchMarker::remove(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
    }

    KillSwitchState disabled;
    disabled.status = KillSwitchStatus::Disabled;
    disabled.backend = m_backend->backendId();
    disabled.recoveryMarkerPresent = false;
    setState(disabled);
    emit logLine(QStringLiteral("helper: kill switch disabled"));
    return true;
}

bool KillSwitchManager::recover(bool force, QString* errorMessage)
{
    Q_UNUSED(force);
    return disable(errorMessage);
}

QString KillSwitchManager::recoveryInstructionsForPlatform()
{
#if defined(Q_OS_LINUX)
    LinuxNftKillSwitchManager backend;
#elif defined(Q_OS_WIN)
    WindowsKillSwitchManager backend;
#elif defined(Q_OS_MACOS)
    MacKillSwitchManager backend;
#else
    StubKillSwitchManager backend(QStringLiteral("unknown"));
#endif
    return backend.recoveryInstructions();
}

KillSwitchRuleSet KillSwitchManager::ruleSetFromJson(const QJsonObject& payload)
{
    KillSwitchRuleSet rules;
    rules.mode = killSwitchModeFromString(payload.value(QStringLiteral("mode")).toString());
    rules.tunInterfaceName =
        payload.value(QStringLiteral("tunInterfaceName")).toString(rules.tunInterfaceName);
    rules.proxyServerHost = payload.value(QStringLiteral("proxyServerHost")).toString();
    rules.proxyServerPort = payload.value(QStringLiteral("proxyServerPort")).toInt(443);

    const auto readIpList = [](const QJsonValue& value) {
        QStringList ips;
        if (value.isArray()) {
            for (const QJsonValue& item : value.toArray()) {
                const QString ip = item.toString().trimmed();
                if (!ip.isEmpty()) {
                    ips.append(ip);
                }
            }
        }
        return ips;
    };

    rules.proxyServerIpv4 = readIpList(payload.value(QStringLiteral("proxyServerIps")));
    if (rules.proxyServerIpv4.isEmpty()) {
        rules.proxyServerIpv4 = readIpList(payload.value(QStringLiteral("proxyServerIpv4")));
    }
    rules.proxyServerIpv6 = readIpList(payload.value(QStringLiteral("proxyServerIpv6")));
    rules.allowLan = payload.value(QStringLiteral("allowLan")).toBool(true);
    rules.allowLoopback = payload.value(QStringLiteral("allowLoopback")).toBool(true);
    rules.blockDirectDns = payload.value(QStringLiteral("blockDirectDns")).toBool(true);
    return rules;
}

QJsonObject KillSwitchManager::stateToJson(const KillSwitchState& state)
{
    QJsonObject object;
    object.insert(QStringLiteral("status"), killSwitchStatusToString(state.status));
    object.insert(QStringLiteral("mode"), killSwitchModeToString(state.mode));
    object.insert(QStringLiteral("backend"), state.backend);
    object.insert(QStringLiteral("lastError"), state.lastError);
    object.insert(QStringLiteral("privileged"), state.privileged);
    object.insert(QStringLiteral("supported"), state.supported);
    object.insert(QStringLiteral("recoveryMarkerPresent"), state.recoveryMarkerPresent);
    if (state.enabledAt.isValid()) {
        object.insert(QStringLiteral("enabledAt"), state.enabledAt.toString(Qt::ISODate));
    }
    QJsonArray rules;
    for (const QString& rule : state.activeRules) {
        rules.append(rule);
    }
    object.insert(QStringLiteral("activeRules"), rules);
    return object;
}

void KillSwitchManager::setState(KillSwitchState state)
{
    m_state = std::move(state);
    emit stateChanged(m_state);
}

} // namespace zarya
