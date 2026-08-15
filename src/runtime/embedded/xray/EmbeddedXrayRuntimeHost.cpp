#include "runtime/embedded/xray/EmbeddedXrayRuntimeHost.h"

#include "diagnostics/DiagnosticsOptions.h"
#include "diagnostics/DiagnosticsRedactor.h"
#include "runtime/core/CoreRuntimeCoordinator.h"
#include "runtime/embedded/xray/bridge/zarya_xray_bridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>

namespace zarya {

namespace {

constexpr int kExpectedAbiVersion = 2;

CoreRuntimeState fromAbiState(int state)
{
    switch (state) {
    case ZARYA_XRAY_STARTING:
        return CoreRuntimeState::Starting;
    case ZARYA_XRAY_RUNNING:
        return CoreRuntimeState::Running;
    case ZARYA_XRAY_STOPPING:
        return CoreRuntimeState::Stopping;
    case ZARYA_XRAY_FAILED:
        return CoreRuntimeState::Failed;
    case ZARYA_XRAY_STOPPED:
    default:
        return CoreRuntimeState::Stopped;
    }
}

} // namespace

EmbeddedXrayRuntimeHost::EmbeddedXrayRuntimeHost(CoreRuntimeCoordinator* coordinator,
                                                 QObject* parent)
    : ICoreRuntimeHost(parent)
    , m_coordinator(coordinator)
{
    m_logTimer.setInterval(200);
    connect(&m_logTimer, &QTimer::timeout, this, &EmbeddedXrayRuntimeHost::drainLogs);
    loadBridge();
}

CoreDistributionKind EmbeddedXrayRuntimeHost::distributionKind() const
{
    return CoreDistributionKind::Embedded;
}

CoreRuntimeCapabilities EmbeddedXrayRuntimeHost::capabilities() const
{
    return CoreRuntimeCapability::Validation;
}

bool EmbeddedXrayRuntimeHost::isAvailable() const
{
    return m_available;
}

QString EmbeddedXrayRuntimeHost::version() const
{
    return m_version;
}

int EmbeddedXrayRuntimeHost::abiVersion() const
{
    return m_abiVersion;
}

QString EmbeddedXrayRuntimeHost::loadStatus() const
{
    return m_loadStatus;
}

QString EmbeddedXrayRuntimeHost::libraryPath() const
{
    return m_libraryPath;
}

CoreRuntimeState EmbeddedXrayRuntimeHost::state() const
{
    return queryState();
}

void EmbeddedXrayRuntimeHost::loadBridge()
{
#if defined(ZARYA_EMBEDDED_XRAY_DISABLED)
    m_loadStatus = QStringLiteral("Embedded Xray was disabled at build time.");
    return;
#elif defined(ZARYA_EMBEDDED_XRAY_STATIC_ABI)
    m_abiVersionFunction = &ZaryaXrayAbiVersion;
    m_versionFunction = &ZaryaXrayVersion;
    m_validateFunction = &ZaryaXrayValidate;
    m_startFunction = &ZaryaXrayStart;
    m_stopFunction = &ZaryaXrayStop;
    m_stateFunction = &ZaryaXrayState;
    m_drainLogsFunction = &ZaryaXrayDrainLogs;
    m_freeFunction = &ZaryaXrayFree;
    m_libraryPath = QStringLiteral("<statically-linked>");
#else
    const QString appDirectory = QDir::cleanPath(QCoreApplication::applicationDirPath());
    m_libraryPath = QDir(appDirectory).absoluteFilePath(
        QStringLiteral(ZARYA_EMBEDDED_XRAY_LIBRARY_NAME));
    const QFileInfo libraryInfo(m_libraryPath);
    if (!libraryInfo.isAbsolute() || QDir::cleanPath(libraryInfo.absolutePath()) != appDirectory) {
        m_loadStatus = QStringLiteral("Embedded Xray path is not an app-owned absolute path.");
        return;
    }
    if (!libraryInfo.isFile()) {
        m_loadStatus = QStringLiteral("Embedded Xray library is missing; repair or reinstall Zarya.");
        return;
    }

    m_library.setFileName(m_libraryPath);
    m_library.setLoadHints(QLibrary::PreventUnloadHint);
    if (!m_library.load()) {
        m_loadStatus = sanitize(QStringLiteral("Embedded Xray failed to load: %1")
                                    .arg(m_library.errorString()));
        return;
    }

    m_abiVersionFunction = reinterpret_cast<AbiVersionFunction>(
        m_library.resolve("ZaryaXrayAbiVersion"));
    m_versionFunction = reinterpret_cast<StringFunction>(m_library.resolve("ZaryaXrayVersion"));
    m_validateFunction = reinterpret_cast<ConfigFunction>(m_library.resolve("ZaryaXrayValidate"));
    m_startFunction = reinterpret_cast<ConfigFunction>(m_library.resolve("ZaryaXrayStart"));
    m_stopFunction = reinterpret_cast<StringFunction>(m_library.resolve("ZaryaXrayStop"));
    m_stateFunction = reinterpret_cast<StateFunction>(m_library.resolve("ZaryaXrayState"));
    m_drainLogsFunction = reinterpret_cast<StringFunction>(m_library.resolve("ZaryaXrayDrainLogs"));
    m_freeFunction = reinterpret_cast<FreeFunction>(m_library.resolve("ZaryaXrayFree"));
#endif

    if (!m_abiVersionFunction || !m_versionFunction || !m_validateFunction || !m_startFunction
        || !m_stopFunction || !m_stateFunction || !m_drainLogsFunction || !m_freeFunction) {
        m_loadStatus = QStringLiteral("Embedded Xray ABI is incomplete; repair or reinstall Zarya.");
        return;
    }
    m_abiVersion = m_abiVersionFunction();
    if (m_abiVersion != kExpectedAbiVersion) {
        m_loadStatus = QStringLiteral("Embedded Xray ABI mismatch (expected %1, got %2).")
                           .arg(kExpectedAbiVersion)
                           .arg(m_abiVersion);
        return;
    }
    m_version = takeString(m_versionFunction());
    m_available = true;
    m_loadStatus = QStringLiteral("Loaded");
}

CoreOperationResult EmbeddedXrayRuntimeHost::validate(const CoreLaunchRequest& request)
{
    if (!m_available) {
        return CoreOperationResult::failure(QStringLiteral("xray.embedded.unavailable"),
                                            m_loadStatus);
    }
    if (state() != CoreRuntimeState::Stopped) {
        return CoreOperationResult::failure(
            QStringLiteral("xray.validation.busy"),
            QStringLiteral("Embedded validation requires the main Xray instance to be stopped."));
    }
    return callConfigFunction(m_validateFunction, request,
                              QStringLiteral("xray.config.invalid"));
}

CoreOperationResult EmbeddedXrayRuntimeHost::start(const CoreLaunchRequest& request)
{
    if (!m_available) {
        return CoreOperationResult::failure(QStringLiteral("xray.embedded.unavailable"),
                                            m_loadStatus);
    }
    QString conflict;
    if (m_coordinator && !m_coordinator->acquire(CoreType::Xray, &conflict)) {
        return CoreOperationResult::failure(QStringLiteral("core.runtime.conflict"), conflict);
    }
    m_coordinatorAcquired = m_coordinator != nullptr;
    setState(CoreRuntimeState::Starting);
    CoreOperationResult result = callConfigFunction(m_startFunction, request,
                                                    QStringLiteral("xray.start.failed"));
    if (!result.success) {
        if (m_coordinatorAcquired) {
            m_coordinator->release(CoreType::Xray);
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

CoreOperationResult EmbeddedXrayRuntimeHost::stop()
{
    if (!m_available) {
        return CoreOperationResult::ok();
    }
    if (state() == CoreRuntimeState::Stopped) {
        if (m_coordinatorAcquired) {
            m_coordinator->release(CoreType::Xray);
            m_coordinatorAcquired = false;
        }
        return CoreOperationResult::ok();
    }
    setState(CoreRuntimeState::Stopping);
    CoreOperationResult result = takeResult(m_stopFunction(), QStringLiteral("xray.stop.failed"));
    drainLogs();
    m_logTimer.stop();
    if (m_coordinatorAcquired) {
        m_coordinator->release(CoreType::Xray);
        m_coordinatorAcquired = false;
    }
    setState(result.success ? CoreRuntimeState::Stopped : CoreRuntimeState::Failed);
    if (!result.success) {
        emit errorOccurred(result.errorCode, result.message);
    }
    return result;
}

CoreOperationResult EmbeddedXrayRuntimeHost::callConfigFunction(
    ConfigFunction function, const CoreLaunchRequest& request, const QString& errorCode)
{
    if (!function) {
        return CoreOperationResult::failure(errorCode,
                                            QStringLiteral("Embedded Xray ABI function is missing."));
    }
    if (request.coreType != CoreType::Xray || request.configJson.isEmpty()) {
        return CoreOperationResult::failure(errorCode,
                                            QStringLiteral("Embedded Xray config is empty."));
    }
    const QByteArray assetPath = request.assetDir.toUtf8();
    char* result = function(request.configJson.constData(),
                            static_cast<std::size_t>(request.configJson.size()),
                            assetPath.constData());
    return takeResult(result, errorCode);
}

CoreOperationResult EmbeddedXrayRuntimeHost::takeResult(char* result,
                                                        const QString& errorCode)
{
    if (!result) {
        return CoreOperationResult::ok();
    }
    const QString message = sanitize(takeString(result));
    return CoreOperationResult::failure(errorCode, message);
}

QString EmbeddedXrayRuntimeHost::takeString(char* value) const
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

QString EmbeddedXrayRuntimeHost::sanitize(const QString& message) const
{
    return DiagnosticsRedactor::redactLogLine(message, DiagnosticsRedactionMode::Basic);
}

void EmbeddedXrayRuntimeHost::drainLogs()
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
        emit errorOccurred(QStringLiteral("xray.runtime.failed"),
                           QStringLiteral("Embedded Xray stopped unexpectedly."));
    }
}

void EmbeddedXrayRuntimeHost::setState(CoreRuntimeState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(state);
}

CoreRuntimeState EmbeddedXrayRuntimeHost::queryState() const
{
    if (!m_available || !m_stateFunction) {
        return m_state;
    }
    return fromAbiState(m_stateFunction());
}

} // namespace zarya
