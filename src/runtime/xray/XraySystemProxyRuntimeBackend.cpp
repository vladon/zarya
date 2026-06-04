#include "runtime/xray/XraySystemProxyRuntimeBackend.h"

#include "storage/AppSettings.h"

#include <QFileInfo>

namespace zarya {

XraySystemProxyRuntimeBackend::XraySystemProxyRuntimeBackend(QObject* parent)
    : IRuntimeBackend(parent)
{
}

void XraySystemProxyRuntimeBackend::setStartHandler(
    std::function<bool(const Profile&, const RuntimeStartOptions&)> handler)
{
    m_startHandler = std::move(handler);
}

void XraySystemProxyRuntimeBackend::setStopHandler(std::function<bool()> handler)
{
    m_stopHandler = std::move(handler);
}

void XraySystemProxyRuntimeBackend::setRunningHandler(std::function<bool()> handler)
{
    m_runningHandler = std::move(handler);
}

QString XraySystemProxyRuntimeBackend::displayName() const
{
    return QStringLiteral("Xray system proxy");
}

RuntimeBackendType XraySystemProxyRuntimeBackend::type() const
{
    return RuntimeBackendType::XraySystemProxy;
}

bool XraySystemProxyRuntimeBackend::isSupported(QString* reason) const
{
    if (!QFileInfo::exists(AppSettings::instance().resolvedXrayPath())) {
        if (reason) {
            *reason = QStringLiteral("Xray executable not found.");
        }
        return false;
    }
    if (reason) {
        reason->clear();
    }
    return true;
}

bool XraySystemProxyRuntimeBackend::validateProfile(const Profile& profile, QString* reason) const
{
    Q_UNUSED(profile);
    if (reason) {
        reason->clear();
    }
    return true;
}

bool XraySystemProxyRuntimeBackend::start(const Profile& profile,
                                          const RuntimeStartOptions& options)
{
    if (!m_startHandler) {
        emit errorOccurred(QStringLiteral("Xray runtime backend is not wired."));
        return false;
    }
    emit logLine(QStringLiteral("Runtime mode: Xray system proxy"));
    return m_startHandler(profile, options);
}

bool XraySystemProxyRuntimeBackend::stop()
{
    if (m_stopHandler) {
        return m_stopHandler();
    }
    return true;
}

bool XraySystemProxyRuntimeBackend::isRunning() const
{
    return m_runningHandler && m_runningHandler();
}

} // namespace zarya
