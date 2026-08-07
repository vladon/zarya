#include "runtime/embedded/singbox/EmbeddedSingBoxRuntimeHost.h"

#include "diagnostics/DiagnosticsOptions.h"
#include "diagnostics/DiagnosticsRedactor.h"
#include "runtime/core/CoreRuntimeCoordinator.h"
#include "runtime/embedded/singbox/bridge/zarya_singbox_bridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>

namespace zarya {

namespace {

constexpr int kExpectedAbiVersion = 1;

CoreRuntimeState fromAbiState(int state)
{
    switch (state) {
    case ZARYA_SINGBOX_STARTING:
        return CoreRuntimeState::Starting;
    case ZARYA_SINGBOX_RUNNING:
        return CoreRuntimeState::Running;
    case ZARYA_SINGBOX_STOPPING:
        return CoreRuntimeState::Stopping;
    case ZARYA_SINGBOX_FAILED:
        return CoreRuntimeState::Failed;
    case ZARYA_SINGBOX_STOPPED:
    default:
        return CoreRuntimeState::Stopped;
    }
}

} // namespace

EmbeddedSingBoxRuntimeHost::EmbeddedSingBoxRuntimeHost(CoreRuntimeCoordinator* coordinator,
                                                 QObject* parent)
    : ICoreRuntimeHost(parent)
    , m_coordinator(coordinator)
{
    m_logTimer.setInterval(200);
    connect(&m_logTimer, &QTimer::timeout, this, &EmbeddedSingBoxRuntimeHost::drainLogs);
    loadBridge();
}

CoreDistributionKind EmbeddedSingBoxRuntimeHost::distributionKind() const
{
    return CoreDistributionKind::Embedded;
}

CoreRuntimeCapabilities EmbeddedSingBoxRuntimeHost::capabilities() const
{
    return CoreRuntimeCapability::Validation;
}

bool EmbeddedSingBoxRuntimeHost::isAvailable() const
{
    return m_available;
}

QString EmbeddedSingBoxRuntimeHost::version() const
{
    return m_version;
}

int EmbeddedSingBoxRuntimeHost::abiVersion() const
{
    return m_abiVersion;
}

QString EmbeddedSingBoxRuntimeHost::loadStatus() const
{
    return m_loadStatus;
}

QString EmbeddedSingBoxRuntimeHost::libraryPath() const
{
    return m_libraryPath;
}

CoreRuntimeState EmbeddedSingBoxRuntimeHost::state() const
{
    return queryState();
}

void EmbeddedSingBoxRuntimeHost::loadBridge()
{
#if defined(ZARYA_EMBEDDED_SINGBOX_DISABLED)
    m_loadStatus = QStringLiteral("Embedded sing-box was disabled at build time.");
    return;
#elif defined(ZARYA_EMBEDDED_SINGBOX_STATIC_ABI)
    m_abiVersionFunction = &ZaryaSingBoxAbiVersion;
    m_versionFunction = &ZaryaSingBoxVersion;
    m_validateFunction = &ZaryaSingBoxValidate;
    m_startFunction = &ZaryaSingBoxStart;
    m_stopFunction = &ZaryaSingBoxStop;
    m_stateFunction = &ZaryaSingBoxState;
    m_drainLogsFunction = &ZaryaSingBoxDrainLogs;
    m_freeFunction = &ZaryaSingBoxFree;
    m_libraryPath = QStringLiteral("<statically-linked>");
#else
    const QString appDirectory = QDir::cleanPath(QCoreApplication::applicationDirPath());
    m_libraryPath = QDir(appDirectory).absoluteFilePath(
        QStringLiteral(ZARYA_EMBEDDED_SINGBOX_LIBRARY_NAME));
    const QFileInfo libraryInfo(m_libraryPath);
    if (!libraryInfo.isAbsolute() || QDir::cleanPath(libraryInfo.absolutePath()) != appDirectory) {
        m_loadStatus = QStringLiteral("Embedded sing-box path is not an app-owned absolute path.");
        return;
    }
    if (!libraryInfo.isFile()) {
        m_loadStatus = QStringLiteral("Embedded sing-box library is missing; repair or reinstall Zarya.");
        return;
    }

    m_library.setFileName(m_libraryPath);
    m_library.setLoadHints(QLibrary::PreventUnloadHint);
    if (!m_library.load()) {
        m_loadStatus = sanitize(QStringLiteral("Embedded sing-box failed to load: %1")
                                    .arg(m_library.errorString()));
        return;
    }

    m_abiVersionFunction = reinterpret_cast<AbiVersionFunction>(
        m_library.resolve("ZaryaSingBoxAbiVersion"));
    m_versionFunction = reinterpret_cast<StringFunction>(m_library.resolve("ZaryaSingBoxVersion"));
    m_validateFunction = reinterpret_cast<ConfigFunction>(m_library.resolve("ZaryaSingBoxValidate"));
    m_startFunction = reinterpret_cast<ConfigFunction>(m_library.resolve("ZaryaSingBoxStart"));
    m_stopFunction = reinterpret_cast<StringFunction>(m_library.resolve("ZaryaSingBoxStop"));
    m_stateFunction = reinterpret_cast<StateFunction>(m_library.resolve("ZaryaSingBoxState"));
    m_drainLogsFunction = reinterpret_cast<StringFunction>(m_library.resolve("ZaryaSingBoxDrainLogs"));
    m_freeFunction = reinterpret_cast<FreeFunction>(m_library.resolve("ZaryaSingBoxFree"));
#endif

    if (!m_abiVersionFunction || !m_versionFunction || !m_validateFunction || !m_startFunction
        || !m_stopFunction || !m_stateFunction || !m_drainLogsFunction || !m_freeFunction) {
        m_loadStatus = QStringLiteral("Embedded sing-box ABI is incomplete; repair or reinstall Zarya.");
        return;
    }
    m_abiVersion = m_abiVersionFunction();
    if (m_abiVersion != kExpectedAbiVersion) {
        m_loadStatus = QStringLiteral("Embedded sing-box ABI mismatch (expected %1, got %2).")
                           .arg(kExpectedAbiVersion)
                           .arg(m_abiVersion);
        return;
    }
    m_version = takeString(m_versionFunction());
    m_available = true;
    m_loadStatus = QStringLiteral("Loaded");
}

CoreOperationResult EmbeddedSingBoxRuntimeHost::validate(const CoreLaunchRequest& request)
{
    if (!m_available) {
        return CoreOperationResult::failure(QStringLiteral("singbox.embedded.unavailable"),
                                            m_loadStatus);
    }
    if (state() != CoreRuntimeState::Stopped) {
        return CoreOperationResult::failure(
            QStringLiteral("singbox.validation.busy"),
            QStringLiteral("Embedded validation requires the main sing-box instance to be stopped."));
    }
    return callConfigFunction(m_validateFunction, request,
                              QStringLiteral("singbox.config.invalid"));
}

CoreOperationResult EmbeddedSingBoxRuntimeHost::start(const CoreLaunchRequest& request)
{
    if (!m_available) {
        return CoreOperationResult::failure(QStringLiteral("singbox.embedded.unavailable"),
                                            m_loadStatus);
    }
    QString conflict;
    if (m_coordinator && !m_coordinator->acquire(CoreType::SingBox, &conflict)) {
        return CoreOperationResult::failure(QStringLiteral("core.runtime.conflict"), conflict);
    }
    m_coordinatorAcquired = m_coordinator != nullptr;
    setState(CoreRuntimeState::Starting);
    CoreOperationResult result = callConfigFunction(m_startFunction, request,
                                                    QStringLiteral("singbox.start.failed"));
    if (!result.success) {
        if (m_coordinatorAcquired) {
            m_coordinator->release(CoreType::SingBox);
            m_coordinatorAcquired = false;
        }
        setState(CoreRuntimeState::Failed);
        emit errorOccurred(result.errorCode, result.message);
        return result;
    }
    setState(CoreRuntimeState::Running);
    m_logTimer.start();
    drainLogs();
    return result;
}

CoreOperationResult EmbeddedSingBoxRuntimeHost::stop()
{
    if (!m_available) {
        return CoreOperationResult::ok();
    }
    if (state() == CoreRuntimeState::Stopped) {
        if (m_coordinatorAcquired) {
            m_coordinator->release(CoreType::SingBox);
            m_coordinatorAcquired = false;
        }
        return CoreOperationResult::ok();
    }
    setState(CoreRuntimeState::Stopping);
    CoreOperationResult result = takeResult(m_stopFunction(), QStringLiteral("singbox.stop.failed"));
    drainLogs();
    m_logTimer.stop();
    if (m_coordinatorAcquired) {
        m_coordinator->release(CoreType::SingBox);
        m_coordinatorAcquired = false;
    }
    setState(result.success ? CoreRuntimeState::Stopped : CoreRuntimeState::Failed);
    if (!result.success) {
        emit errorOccurred(result.errorCode, result.message);
    }
    return result;
}

CoreOperationResult EmbeddedSingBoxRuntimeHost::callConfigFunction(
    ConfigFunction function, const CoreLaunchRequest& request, const QString& errorCode)
{
    if (!function) {
        return CoreOperationResult::failure(errorCode,
                                            QStringLiteral("Embedded sing-box ABI function is missing."));
    }
    if (request.coreType != CoreType::SingBox || request.configJson.isEmpty()) {
        return CoreOperationResult::failure(errorCode,
                                            QStringLiteral("Embedded sing-box config is empty."));
    }
    char* result = function(request.configJson.constData(),
                            static_cast<std::size_t>(request.configJson.size()));
    return takeResult(result, errorCode);
}

CoreOperationResult EmbeddedSingBoxRuntimeHost::takeResult(char* result,
                                                        const QString& errorCode)
{
    if (!result) {
        return CoreOperationResult::ok();
    }
    const QString message = sanitize(takeString(result));
    return CoreOperationResult::failure(errorCode, message);
}

QString EmbeddedSingBoxRuntimeHost::takeString(char* value) const
{
    if (!value) {
        return {};
    }
    const QString result = QString::fromUtf8(value);
    if (m_freeFunction) {
        m_freeFunction(value);
    }
    return result;
}

QString EmbeddedSingBoxRuntimeHost::sanitize(const QString& message) const
{
    return DiagnosticsRedactor::redactLogLine(message, DiagnosticsRedactionMode::Basic);
}

void EmbeddedSingBoxRuntimeHost::drainLogs()
{
    if (!m_available || !m_drainLogsFunction) {
        return;
    }
    const QString logs = takeString(m_drainLogsFunction());
    for (const QString& line : logs.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        emit logLine(sanitize(line.trimmed()));
    }
    const CoreRuntimeState actual = queryState();
    if (m_state == CoreRuntimeState::Running && actual == CoreRuntimeState::Failed) {
        m_logTimer.stop();
        setState(CoreRuntimeState::Failed);
        emit errorOccurred(QStringLiteral("singbox.runtime.failed"),
                           QStringLiteral("Embedded sing-box stopped unexpectedly."));
    }
}

void EmbeddedSingBoxRuntimeHost::setState(CoreRuntimeState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(state);
}

CoreRuntimeState EmbeddedSingBoxRuntimeHost::queryState() const
{
    if (!m_available || !m_stateFunction) {
        return m_state;
    }
    return fromAbiState(m_stateFunction());
}

} // namespace zarya
