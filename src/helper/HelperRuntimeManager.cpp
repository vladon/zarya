#include "helper/HelperRuntimeManager.h"

namespace zarya {

HelperRuntimeManager::HelperRuntimeManager(QObject* parent)
    : QObject(parent)
    , m_runtimeHost(&m_coordinator, this)
{
    connect(&m_runtimeHost, &ICoreRuntimeHost::logLine, this, &HelperRuntimeManager::logLine);
    connect(&m_runtimeHost, &ICoreRuntimeHost::errorOccurred, this,
            [this](const QString& code, const QString& message) {
                emit logLine(QStringLiteral("helper embedded sing-box %1: %2").arg(code, message));
            });
    connect(&m_runtimeHost, &ICoreRuntimeHost::stateChanged, this,
            [this](CoreRuntimeState state) {
                if (state == CoreRuntimeState::Failed) {
                    emit runtimeExited(-1);
                }
            });
}

bool HelperRuntimeManager::isRunning() const
{
    return m_runtimeHost.state() == CoreRuntimeState::Running;
}

QString HelperRuntimeManager::version() const
{
    return m_runtimeHost.version();
}

int HelperRuntimeManager::abiVersion() const
{
    return m_runtimeHost.abiVersion();
}

QString HelperRuntimeManager::loadStatus() const
{
    return m_runtimeHost.loadStatus();
}

QDateTime HelperRuntimeManager::startedAt() const
{
    return m_startedAt;
}

CoreLaunchRequest HelperRuntimeManager::makeRequest(const QByteArray& configJson) const
{
    CoreLaunchRequest request;
    request.coreType = CoreType::SingBox;
    request.configJson = configJson;
    return request;
}

bool HelperRuntimeManager::validateConfig(const QByteArray& configJson, QString* output,
                                          QString* errorMessage)
{
    const CoreOperationResult result = m_runtimeHost.validate(makeRequest(configJson));
    if (output) {
        *output = result.success ? QStringLiteral("Embedded sing-box config validation OK")
                                 : result.message;
    }
    if (!result.success && errorMessage) {
        *errorMessage = result.message;
    }
    return result.success;
}

bool HelperRuntimeManager::startTun(const QByteArray& configJson, bool checkBeforeStart,
                                    QString* errorMessage)
{
    if (isRunning()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Embedded sing-box is already running.");
        }
        return false;
    }
    if (!m_runtimeHost.isAvailable()) {
        if (errorMessage) {
            *errorMessage = m_runtimeHost.loadStatus();
        }
        return false;
    }
    if (checkBeforeStart && !validateConfig(configJson, nullptr, errorMessage)) {
        return false;
    }
    emit logLine(QStringLiteral("helper: starting embedded sing-box"));
    const CoreOperationResult result = m_runtimeHost.start(makeRequest(configJson));
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = result.message;
        }
        return false;
    }
    m_startedAt = QDateTime::currentDateTimeUtc();
    return true;
}

bool HelperRuntimeManager::compileRuleSet(const QByteArray& ruleSetJson,
                                          const QString& outputPath,
                                          QString* errorMessage)
{
    const CoreOperationResult result = m_runtimeHost.compileRuleSet(ruleSetJson, outputPath);
    if (!result.success && errorMessage) {
        *errorMessage = result.message;
    }
    return result.success;
}
bool HelperRuntimeManager::stopTun(QString* errorMessage)
{
    emit logLine(QStringLiteral("helper: stopping embedded sing-box"));
    const CoreOperationResult result = m_runtimeHost.stop();
    if (!result.success && errorMessage) {
        *errorMessage = result.message;
    }
    if (result.success) {
        m_startedAt = {};
        emit logLine(QStringLiteral("helper: embedded sing-box stopped"));
    }
    return result.success;
}

} // namespace zarya