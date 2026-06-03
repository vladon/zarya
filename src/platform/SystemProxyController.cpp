#include "platform/SystemProxyController.h"

#include "platform/SystemProxyDebug.h"
#include "platform/SystemProxyManagerFactory.h"

namespace zarya {

namespace {

QString uiStatusToString(SystemProxyUiStatus status)
{
    switch (status) {
    case SystemProxyUiStatus::Unsupported:
        return QStringLiteral("unsupported");
    case SystemProxyUiStatus::Partial:
        return QStringLiteral("partial");
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
    if (manager->supportLevel() == QStringLiteral("partial")) {
        return SystemProxyUiStatus::Partial;
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
    if (m_uiStatus == SystemProxyUiStatus::Partial && m_manager) {
        return QStringLiteral("%1 (%2)").arg(status, m_manager->backendName());
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
        logLine(QStringLiteral("System proxy unsupported on this platform."));
        return;
    }

    logLine(QStringLiteral("System proxy backend: %1").arg(m_manager->backendName()));
    if (!m_manager->limitations().isEmpty()) {
        logLine(m_manager->limitations());
    }

    if (!m_manager->isSupported()) {
        logLine(QStringLiteral("System proxy unsupported: %1").arg(m_manager->limitations()));
        return;
    }

    logLine(QStringLiteral("Reading current proxy state…"));
    QString error;
    const SystemProxyState state = m_manager->readCurrentState(&error);
    if (!error.isEmpty()) {
        logLine(QStringLiteral("Failed to read proxy settings: %1").arg(error));
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

    logLine(QStringLiteral("Reading current proxy state…"));
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
    logLine(QStringLiteral("Previous proxy state saved."));
    logLine(formatSystemProxyStateForLog(m_savedState));
    return true;
}

bool SystemProxyController::enableLocalHttpProxy(
    int port, const std::function<void(const QString&)>& logLine, QString* errorMessage)
{
    if (!m_manager) {
        m_lastError = QStringLiteral("System proxy unsupported on this platform.");
        m_uiStatus = SystemProxyUiStatus::Unsupported;
        logLine(m_lastError);
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        return false;
    }

    logLine(QStringLiteral("System proxy backend: %1").arg(m_manager->backendName()));

    if (m_manager->supportLevel() == QStringLiteral("partial")) {
        m_lastError = m_manager->limitations();
        m_uiStatus = SystemProxyUiStatus::Partial;
        logLine(QStringLiteral("KDE proxy support is partial/unsupported"));
        logLine(m_lastError);
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        return false;
    }

    if (!isSupported()) {
        m_lastError = m_manager->limitations().isEmpty()
                          ? QStringLiteral("System proxy unsupported on this platform.")
                          : m_manager->limitations();
        m_uiStatus = SystemProxyUiStatus::Unsupported;
        logLine(QStringLiteral("System proxy unsupported: %1").arg(m_lastError));
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
    logLine(QStringLiteral("Applying HTTP/HTTPS proxy %1:%2").arg(host).arg(port));

    if (m_manager->backendName().contains(QStringLiteral("macOS"), Qt::CaseInsensitive)) {
        for (const QString& service : m_savedState.affectedNetworkServices) {
            logLine(QStringLiteral("Applying to macOS service: %1").arg(service));
        }
    } else if (m_manager->backendName().contains(QStringLiteral("GNOME"), Qt::CaseInsensitive)) {
        logLine(QStringLiteral("Applying GNOME proxy settings"));
    }

    QString error;
    if (!m_manager->applyHttpProxy(host, port, &error)) {
        m_lastError = error;
        m_uiStatus = SystemProxyUiStatus::Failed;
        logLine(QStringLiteral("Failed to apply system proxy: %1").arg(error));
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    m_enabledByZarya = true;
    m_uiStatus = SystemProxyUiStatus::On;
    m_lastError.clear();
    logLine(QStringLiteral("System proxy applied successfully."));
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
            QStringLiteral("No saved previous proxy state. Nothing to restore.");
        m_lastError = message;
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    }

    if (!m_hasSavedState) {
        const QString message =
            QStringLiteral("Previous proxy state is missing. Cannot restore.");
        m_lastError = message;
        m_uiStatus = SystemProxyUiStatus::Failed;
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    }

    if (!isSupported()) {
        m_lastError = QStringLiteral("System proxy unsupported on this platform.");
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        return false;
    }

    logLine(QStringLiteral("Restoring previous proxy state…"));
    logLine(formatSystemProxyStateForLog(m_savedState));

    QString error;
    if (!m_manager->restoreState(m_savedState, &error)) {
        m_lastError = error;
        m_uiStatus = SystemProxyUiStatus::Failed;
        logLine(QStringLiteral("Proxy restore failed: %1").arg(error));
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    logLine(QStringLiteral("Proxy restore success."));
    clearRuntimeState();
    return true;
}

void SystemProxyController::clearRuntimeState()
{
    m_hasSavedState = false;
    m_enabledByZarya = false;
    m_savedState = {};
    m_lastError.clear();
    m_uiStatus = initialUiStatus(m_manager.get());
}

} // namespace zarya
