#include "testing/RealDelayTester.h"

#include "core/XrayAdapter.h"
#include "domain/RoutingProfile.h"
#include "platform/KillOnCloseProcessJob.h"
#include "storage/AppPaths.h"
#include "testing/PortAllocator.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace zarya {

namespace {

void logLine(const RealDelayTester::LogCallback& log, const QString& line)
{
    if (log) {
        log(line);
    }
}

QString workerPath()
{
#if defined(Q_OS_WIN)
    constexpr auto workerName = "zarya-core-test-worker.exe";
#else
    constexpr auto workerName = "zarya-core-test-worker";
#endif
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(
        QString::fromLatin1(workerName));
}

TestStatus statusFromError(const QString& errorCode)
{
    return errorCode.contains(QStringLiteral("timeout"), Qt::CaseInsensitive)
        ? TestStatus::Timeout
        : TestStatus::Failed;
}

} // namespace

TestResult RealDelayTester::run(const Profile& profile, int timeoutMs, const QString& testUrl,
                                const LogCallback& log, const std::atomic<bool>* cancelFlag)
{
    TestResult result;
    XrayAdapter adapter;
    QString unsupportedReason;
    if (!adapter.supportsProfile(profile, &unsupportedReason)) {
        result.status = TestStatus::Unsupported;
        result.errorMessage = unsupportedReason;
        return result;
    }

    const InboundPorts ports = PortAllocator::allocateInboundPorts();
    if (ports.mixedPort < 1) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral("Failed to allocate local port for test.");
        return result;
    }

    XrayInboundPorts inboundPorts;
    inboundPorts.mixedPort = ports.mixedPort;
    const ConfigGenerationResult generation =
        adapter.generateConfig(profile, inboundPorts, RoutingProfile::builtInProxyAll());
    if (!generation.success) {
        result.status = TestStatus::Failed;
        result.errorMessage = generation.errorMessage;
        return result;
    }

    const QString executable = workerPath();
    if (!QFileInfo::exists(executable)) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral(
            "Embedded core test worker is missing; repair or reinstall Zarya.");
        return result;
    }

    QJsonObject request{
        {QStringLiteral("operation"), QStringLiteral("realDelay")},
        {QStringLiteral("config"), generation.config},
        {QStringLiteral("assetDir"), AppPaths::xrayCoreDir()},
        {QStringLiteral("dataDir"), AppPaths::testRuntimeDir()},
        {QStringLiteral("proxyPort"), ports.mixedPort},
        {QStringLiteral("url"), testUrl},
        {QStringLiteral("timeoutMs"), timeoutMs},
    };

    QProcess worker;
    worker.setProgram(executable);
    logLine(log, QStringLiteral("Starting isolated embedded Xray test worker."));
    worker.start();
    if (!worker.waitForStarted(5000)) {
        result.status = TestStatus::Failed;
        result.errorMessage = worker.errorString();
        return result;
    }

    KillOnCloseProcessJob killJob;
    if (!killJob.attach(worker.processId())) {
        logLine(log, QStringLiteral("Warning: test worker kill-on-close attachment failed."));
    }
    worker.write(QJsonDocument(request).toJson(QJsonDocument::Compact));
    worker.closeWriteChannel();

    const int processTimeoutMs = qBound(5000, timeoutMs + 10000, 70000);
    int waitedMs = 0;
    while (worker.state() != QProcess::NotRunning && waitedMs < processTimeoutMs) {
        if (cancelFlag && cancelFlag->load()) {
            worker.kill();
            worker.waitForFinished(3000);
            killJob.reset();
            result.status = TestStatus::Canceled;
            result.errorMessage = QStringLiteral("Test canceled.");
            return result;
        }
        worker.waitForFinished(100);
        waitedMs += 100;
    }
    if (worker.state() != QProcess::NotRunning) {
        worker.kill();
        worker.waitForFinished(3000);
        killJob.reset();
        result.status = TestStatus::Timeout;
        result.errorMessage = QStringLiteral("Embedded core test worker timed out.");
        return result;
    }
    killJob.reset();

    const QByteArray output = worker.readAllStandardOutput().trimmed();
    QJsonParseError parseError;
    const QJsonDocument response = QJsonDocument::fromJson(output, &parseError);
    if (!response.isObject()) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral("Invalid core test worker response: %1")
                                  .arg(parseError.errorString());
        return result;
    }

    const QJsonObject object = response.object();
    const bool success = object.value(QStringLiteral("success")).toBool();
    if (!success) {
        const QString errorCode = object.value(QStringLiteral("errorCode")).toString();
        result.status = statusFromError(errorCode);
        result.errorMessage = object.value(QStringLiteral("message")).toString();
        return result;
    }

    result.status = TestStatus::Available;
    result.realDelayMs = object.value(QStringLiteral("delayMs")).toInt(-1);
    logLine(log, QStringLiteral("Real delay OK: %1 ms").arg(result.realDelayMs));
    return result;
}

} // namespace zarya
