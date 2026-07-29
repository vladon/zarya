#include "platform/SystemProxyController.h"

#include "i18n/ZaryaTr.h"
#include "platform/SystemProxyDebug.h"
#include "platform/SystemProxyManagerFactory.h"
#include "platform/SystemProxyStateStore.h"

namespace zarya {

namespace {

QString uiStatusToString(SystemProxyUiStatus status)
{
    switch (status) {
    case SystemProxyUiStatus::Unsupported:
        return QStringLiteral("unsupported");
    case SystemProxyUiStatus::Off:
        return QStringLiteral("off");
    case SystemProxyUiStatus::On:
        return QStringLiteral("on");
    case SystemProxyUiStatus::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("off");
}

SystemProxyUiStatus initialUiStatus(const ISystemProxyManager* manager)
{
    if (!manager) {
        return SystemProxyUiStatus::Unsupported;
    }
    return manager->isSupported() ? SystemProxyUiStatus::Off : SystemProxyUiStatus::Unsupported;
}

} // namespace

SystemProxyController::SystemProxyController()
    : m_manager(SystemProxyManagerFactory::create())
{
    m_uiStatus = initialUiStatus(m_manager.get());
}

bool SystemProxyController::isSupported() const
{
    return m_manager && m_manager->isSupported();
}

QString SystemProxyController::backendName() const
{
    return m_manager ? m_manager->backendName() : QStringLiteral("Unsupported");
}

QString SystemProxyController::supportLevel() const
{
    return m_manager ? m_manager->supportLevel() : QStringLiteral("unsupported");
}

QString SystemProxyController::limitations() const
{
    return m_manager ? m_manager->limitations() : QString();
}

SystemProxyUiStatus SystemProxyController::uiStatus() const
{
    return m_uiStatus;
}

QString SystemProxyController::uiStatusText() const
{
    const QString status = uiStatusToString(m_uiStatus);
    if (m_uiStatus == SystemProxyUiStatus::On && m_manager) {
        return QStringLiteral("%1 via %2").arg(status, m_manager->backendName());
    }
    return status;
}

bool SystemProxyController::hasSavedState() const
{
    return m_hasSavedState;
}

bool SystemProxyController::enabledByZarya() const
{
    return m_enabledByZarya;
}

QString SystemProxyController::lastError() const
{
    return m_lastError;
}

void SystemProxyController::logCurrentState(const std::function<void(const QString&)>& logLine) const
{
    if (!m_manager) {
        logLine(ZaryaTr::tr("System proxy is not supported on this platform."));
        return;
    }

    logLine(ZaryaTr::tr("System proxy backend: %1").arg(m_manager->backendName()));
    if (!m_manager->limitations().isEmpty()) {
        logLine(m_manager->limitations());
    }

    if (!m_manager->isSupported()) {
        logLine(ZaryaTr::tr("System proxy unavailable: %1").arg(m_manager->limitations()));
        return;
    }

    logLine(ZaryaTr::tr("Reading current proxy state…"));
    QString error;
    const SystemProxyState state = m_manager->readCurrentState(&error);
    if (!error.isEmpty()) {
        logLine(ZaryaTr::tr("Failed to read proxy settings: %1").arg(error));
        return;
    }

    logLine(formatSystemProxyStateForLog(state));
}

bool SystemProxyController::ensurePreviousStateSaved(
    const std::function<void(const QString&)>& logLine, QString* errorMessage)
{
    if (m_hasSavedState || m_enabledByZarya) {
        return true;
    }

    logLine(ZaryaTr::tr("Reading current proxy state…"));
    QString error;
    m_savedState = m_manager->readCurrentState(&error);
    if (!error.isEmpty()) {
        m_lastError = error;
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    m_hasSavedState = true;
    SystemProxyStateStore::save(m_savedState);
    logLine(ZaryaTr::tr("Previous proxy state saved."));
    logLine(formatSystemProxyStateForLog(m_savedState));
    return true;
}

bool SystemProxyController::enableLocalHttpProxy(
    int port, const std::function<void(const QString&)>& logLine, QString* errorMessage)
{
    if (!m_manager) {
        m_lastError = ZaryaTr::tr("System proxy is not supported on this platform.");
        m_uiStatus = SystemProxyUiStatus::Unsupported;
        logLine(m_lastError);
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        return false;
    }

    logLine(ZaryaTr::tr("System proxy backend: %1").arg(m_manager->backendName()));

    if (!isSupported()) {
        m_lastError = m_manager->limitations().isEmpty()
                          ? ZaryaTr::tr("System proxy is not supported on this platform.")
                          : m_manager->limitations();
        m_uiStatus = SystemProxyUiStatus::Unsupported;
        logLine(ZaryaTr::tr("System proxy unavailable: %1").arg(m_lastError));
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        return false;
    }

    if (!ensurePreviousStateSaved(logLine, errorMessage)) {
        m_uiStatus = SystemProxyUiStatus::Failed;
        return false;
    }

    const QString host = QStringLiteral("127.0.0.1");
    logLine(ZaryaTr::tr("Applying HTTP/HTTPS proxy %1:%2").arg(host).arg(port));
    logLine(ZaryaTr::tr("Applying proxy settings via %1").arg(m_manager->backendName()));

    QString error;
    if (!m_manager->applyHttpProxy(host, port, &error)) {
        m_lastError = error;
        m_uiStatus = SystemProxyUiStatus::Failed;
        logLine(ZaryaTr::tr("Failed to apply system proxy: %1").arg(error));
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    m_enabledByZarya = true;
    m_uiStatus = SystemProxyUiStatus::On;
    m_lastError.clear();
    logLine(ZaryaTr::tr("System proxy applied successfully."));
    return true;
}

bool SystemProxyController::restorePreviousProxy(SystemProxyRestoreMode mode,
                                                 const std::function<void(const QString&)>& logLine,
                                                 QString* errorMessage)
{
    if (mode == SystemProxyRestoreMode::Automatic && !m_enabledByZarya) {
        return true;
    }

    if (mode == SystemProxyRestoreMode::Manual && !m_hasSavedState) {
        const QString message =
            ZaryaTr::tr("No saved previous proxy state. Nothing to restore.");
        m_lastError = message;
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    }

    if (!m_hasSavedState) {
        const QString message =
            ZaryaTr::tr("Previous proxy state is missing. Cannot restore.");
        m_lastError = message;
        m_uiStatus = SystemProxyUiStatus::Failed;
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    }

    if (!isSupported()) {
        m_lastError = ZaryaTr::tr("System proxy is not supported on this platform.");
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        return false;
    }

    logLine(ZaryaTr::tr("Restoring previous proxy state…"));
    logLine(formatSystemProxyStateForLog(m_savedState));

    QString error;
    if (!m_manager->restoreState(m_savedState, &error)) {
        m_lastError = error;
        m_uiStatus = SystemProxyUiStatus::Failed;
        logLine(ZaryaTr::tr("Proxy restore failed: %1").arg(error));
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    logLine(ZaryaTr::tr("Proxy restore succeeded."));
    clearRuntimeState();
    return true;
}

bool SystemProxyController::restorePersistedPreviousProxy(
    const std::function<void(const QString&)>& logLine, QString* errorMessage)
{
    SystemProxyState persisted;
    if (!SystemProxyStateStore::load(&persisted)) {
        if (errorMessage) {
            *errorMessage = ZaryaTr::tr("No persisted previous proxy state.");
        }
        return false;
    }

    m_savedState = persisted;
    m_hasSavedState = true;
    m_enabledByZarya = true;
    return restorePreviousProxy(SystemProxyRestoreMode::Manual, logLine, errorMessage);
}

bool SystemProxyController::tryClearZaryaOwnedProxy(
    int httpPort, const std::function<void(const QString&)>& logLine, QString* errorMessage)
{
    if (!m_manager || !m_manager->isSupported()) {
        if (errorMessage) {
            *errorMessage = ZaryaTr::tr("System proxy is not supported on this platform.");
        }
        return false;
    }

    QString readError;
    const SystemProxyState current = m_manager->readCurrentState(&readError);
    if (!readError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = readError;
        }
        return false;
    }

    const QString expected = QStringLiteral("127.0.0.1:%1").arg(httpPort);
    if (!current.proxyEnabled || current.proxyServer.trimmed() != expected) {
        if (errorMessage) {
            *errorMessage =
                ZaryaTr::tr("Current proxy does not match Zarya HTTP endpoint.");
        }
        return false;
    }

    SystemProxyState disabled = current;
    disabled.proxyEnabled = false;
    disabled.proxyServer.clear();
    logLine(ZaryaTr::tr("Clearing Zarya-owned proxy %1").arg(expected));
    QString applyError;
    if (!m_manager->restoreState(disabled, &applyError)) {
        if (errorMessage) {
            *errorMessage = applyError;
        }
        return false;
    }
    clearRuntimeState();
    return true;
}

void SystemProxyController::clearRuntimeState()
{
    m_hasSavedState = false;
    m_enabledByZarya = false;
    m_savedState = {};
    m_lastError.clear();
    SystemProxyStateStore::clear();
    m_uiStatus = initialUiStatus(m_manager.get());
}

} // namespace zarya
