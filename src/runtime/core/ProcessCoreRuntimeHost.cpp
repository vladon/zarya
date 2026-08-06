#include "runtime/core/ProcessCoreRuntimeHost.h"

#include "core/CoreManager.h"
#include "domain/CoreType.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace zarya {

ProcessCoreRuntimeHost::ProcessCoreRuntimeHost(CoreManager* manager,
                                               CoreDistributionKind distributionKind,
                                               ExecutableResolver executableResolver,
                                               QObject* parent)
    : ICoreRuntimeHost(parent)
    , m_manager(manager)
    , m_distributionKind(distributionKind)
    , m_executableResolver(std::move(executableResolver))
{
    if (!m_manager) {
        return;
    }
    connect(m_manager, &CoreManager::started, this, [this](const QString&) {
        m_state = CoreRuntimeState::Running;
        emit stateChanged(m_state);
    });
    connect(m_manager, &CoreManager::stopped, this, [this]() {
        m_state = CoreRuntimeState::Stopped;
        emit stateChanged(m_state);
    });
    connect(m_manager, &CoreManager::logLine, this, &ICoreRuntimeHost::logLine);
    connect(m_manager, &CoreManager::errorOccurred, this, [this](const QString& message) {
        m_state = CoreRuntimeState::Failed;
        emit stateChanged(m_state);
        emit errorOccurred(QStringLiteral("core.process.failed"), message);
    });
}

CoreDistributionKind ProcessCoreRuntimeHost::distributionKind() const
{
    return m_distributionKind;
}

CoreRuntimeCapabilities ProcessCoreRuntimeHost::capabilities() const
{
    return CoreRuntimeCapability::Validation;
}

bool ProcessCoreRuntimeHost::isAvailable() const
{
    return m_manager && m_executableResolver;
}

QString ProcessCoreRuntimeHost::version() const
{
    return {};
}

int ProcessCoreRuntimeHost::abiVersion() const
{
    return 0;
}

QString ProcessCoreRuntimeHost::loadStatus() const
{
    return isAvailable() ? QStringLiteral("Available")
                         : QStringLiteral("Process runtime host is not configured.");
}

CoreRuntimeState ProcessCoreRuntimeHost::state() const
{
    return m_state;
}

CoreOperationResult ProcessCoreRuntimeHost::materializeConfig(
    const CoreLaunchRequest& request, QString* path) const
{
    if (!path || request.configJson.isEmpty() || request.dataDir.trimmed().isEmpty()) {
        return CoreOperationResult::failure(QStringLiteral("core.process.config.invalid"),
                                            QStringLiteral("Process core config is empty."));
    }
    QDir directory(request.dataDir);
    if (!directory.mkpath(QStringLiteral("."))) {
        return CoreOperationResult::failure(QStringLiteral("core.process.config.write"),
                                            QStringLiteral("Could not create the runtime directory."));
    }
    *path = directory.absoluteFilePath(
        QStringLiteral("%1-runtime.json").arg(coreTypeToString(request.coreType).toLower()));
    QFile file(*path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return CoreOperationResult::failure(QStringLiteral("core.process.config.write"),
                                            file.errorString());
    }
    if (file.write(request.configJson) != request.configJson.size()) {
        return CoreOperationResult::failure(QStringLiteral("core.process.config.write"),
                                            file.errorString());
    }
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return CoreOperationResult::ok();
}

QString ProcessCoreRuntimeHost::executablePath(CoreType type) const
{
    return m_executableResolver ? m_executableResolver(type) : QString();
}

CoreOperationResult ProcessCoreRuntimeHost::validate(const CoreLaunchRequest& request)
{
    const QString executable = executablePath(request.coreType);
    if (!QFileInfo::exists(executable)) {
        return CoreOperationResult::failure(QStringLiteral("core.process.missing"),
                                            QStringLiteral("Core executable is missing."));
    }
    QString configPath;
    CoreOperationResult materialized = materializeConfig(request, &configPath);
    if (!materialized.success) {
        return materialized;
    }
    const CoreValidationResult validation = request.coreType == CoreType::SingBox
        ? m_manager->validateSingBoxConfig(executable, configPath)
        : m_manager->validateConfig(executable, configPath);
    if (!validation.success) {
        return CoreOperationResult::failure(QStringLiteral("core.process.config.invalid"),
                                            validation.errorMessage);
    }
    return CoreOperationResult::ok();
}

CoreOperationResult ProcessCoreRuntimeHost::start(const CoreLaunchRequest& request)
{
    if (m_manager->isRunning()) {
        return CoreOperationResult::failure(QStringLiteral("core.runtime.conflict"),
                                            QStringLiteral("A core process is already running."));
    }
    const QString executable = executablePath(request.coreType);
    if (!QFileInfo::exists(executable)) {
        return CoreOperationResult::failure(QStringLiteral("core.process.missing"),
                                            QStringLiteral("Core executable is missing."));
    }
    CoreOperationResult materialized = materializeConfig(request, &m_activeConfigPath);
    if (!materialized.success) {
        return materialized;
    }
    m_state = CoreRuntimeState::Starting;
    emit stateChanged(m_state);
    if (request.coreType == CoreType::SingBox) {
        m_manager->start(executable, QStringLiteral("sing-box"),
                         {QStringLiteral("run"), QStringLiteral("-c"), m_activeConfigPath});
    } else {
        m_manager->startCore(executable, m_activeConfigPath, QStringLiteral("Xray"));
    }
    return m_manager->isRunning()
        ? CoreOperationResult::ok()
        : CoreOperationResult::failure(QStringLiteral("core.process.start"),
                                       QStringLiteral("Core process did not start."));
}

CoreOperationResult ProcessCoreRuntimeHost::stop()
{
    if (!m_manager || !m_manager->isRunning()) {
        m_state = CoreRuntimeState::Stopped;
        return CoreOperationResult::ok();
    }
    m_state = CoreRuntimeState::Stopping;
    emit stateChanged(m_state);
    m_manager->stop();
    return CoreOperationResult::ok();
}

} // namespace zarya
