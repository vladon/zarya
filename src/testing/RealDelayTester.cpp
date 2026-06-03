#include "testing/RealDelayTester.h"

#include "core/XrayAdapter.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "testing/PortAllocator.h"

#include <QDateTime>
#include <QEventLoop>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace zarya {

namespace {

void logLine(const RealDelayTester::LogCallback& log, const QString& line)
{
    if (log) {
        log(line);
    }
}

bool waitForLocalPort(int port, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, static_cast<quint16>(port));
        if (socket.waitForConnected(200)) {
            socket.disconnectFromHost();
            return true;
        }
        QThread::msleep(50);
    }
    return false;
}

bool isSuccessHttpStatus(int statusCode)
{
    return statusCode == 204 || statusCode == 200 || statusCode == 301 || statusCode == 302;
}

QString writeConfigFile(const Profile& profile, const QJsonObject& config)
{
    const QString timestamp =
        QString::number(QDateTime::currentMSecsSinceEpoch());
    const QString path =
        QDir(AppPaths::testRuntimeDir())
            .filePath(QStringLiteral("config-%1-%2.json").arg(profile.id, timestamp));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    file.write(QJsonDocument(config).toJson(QJsonDocument::Indented));
    return path;
}

struct ValidationResult {
    bool success = false;
    int exitCode = -1;
    QString output;
    QString errorMessage;
};

ValidationResult validateConfig(const QString& executablePath, const QString& configPath)
{
    ValidationResult result;
    QProcess process;
    process.setProgram(executablePath);
    process.setArguments({QStringLiteral("run"), QStringLiteral("-test"),
                          QStringLiteral("-config"), configPath});
    process.start();
    if (!process.waitForStarted(5000)) {
        result.errorMessage = process.errorString();
        return result;
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(1000);
        result.errorMessage = QStringLiteral("Xray config validation timed out.");
        return result;
    }
    result.exitCode = process.exitCode();
    result.output =
        QString::fromUtf8(process.readAllStandardOutput() + process.readAllStandardError())
            .trimmed();
    result.success = result.exitCode == 0;
    if (!result.success) {
        result.errorMessage = result.output.isEmpty()
                                  ? QStringLiteral("Xray config validation failed.")
                                  : result.output;
    }
    return result;
}

} // namespace

TestResult RealDelayTester::run(const Profile& profile, int timeoutMs, const QString& testUrl,
                                const LogCallback& log)
{
    TestResult result;
    const AppSettings& settings = AppSettings::instance();
    const QString executablePath = settings.resolvedXrayPath();

    if (executablePath.trimmed().isEmpty() || !QFileInfo::exists(executablePath)) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral("Xray executable path is not configured.");
        return result;
    }

    XrayAdapter adapter;
    QString unsupportedReason;
    if (!adapter.supportsProfile(profile, &unsupportedReason)) {
        result.status = TestStatus::Unsupported;
        result.errorMessage = unsupportedReason;
        return result;
    }

    const InboundPorts ports = PortAllocator::allocateInboundPorts();
    if (ports.socksPort < 1 || ports.httpPort < 1) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral("Failed to allocate local ports for test.");
        return result;
    }

    XrayInboundPorts inboundPorts;
    inboundPorts.socksPort = ports.socksPort;
    inboundPorts.httpPort = ports.httpPort;

    const ConfigGenerationResult generation = adapter.generateConfig(profile, inboundPorts);
    if (!generation.success) {
        result.status = TestStatus::Failed;
        result.errorMessage = generation.errorMessage;
        return result;
    }

    const QString configPath = writeConfigFile(profile, generation.config);
    if (configPath.isEmpty()) {
        result.status = TestStatus::Failed;
        result.errorMessage = QStringLiteral("Failed to write temporary Xray config.");
        return result;
    }

    logLine(log, QStringLiteral("Temporary config path: %1").arg(configPath));
    logLine(log, QStringLiteral("Temporary HTTP proxy: 127.0.0.1:%1").arg(ports.httpPort));
    logLine(log, QStringLiteral("Validating Xray config…"));

    const ValidationResult validation = validateConfig(executablePath, configPath);
    if (!validation.output.isEmpty()) {
        logLine(log, validation.output);
    }
    if (!validation.success) {
        result.status = TestStatus::Failed;
        result.errorMessage = validation.errorMessage;
        QFile::remove(configPath);
        return result;
    }
    logLine(log, QStringLiteral("Validation OK"));

    QProcess xrayProcess;
    xrayProcess.setProgram(executablePath);
    xrayProcess.setArguments(adapter.argumentsForConfig(configPath));
    logLine(log, QStringLiteral("Starting temporary Xray…"));
    xrayProcess.start();

    auto stopXray = [&xrayProcess]() {
        if (xrayProcess.state() == QProcess::NotRunning) {
            return;
        }
        xrayProcess.terminate();
        if (!xrayProcess.waitForFinished(3000)) {
            xrayProcess.kill();
            xrayProcess.waitForFinished(1000);
        }
    };

    if (!xrayProcess.waitForStarted(5000)) {
        result.status = TestStatus::Failed;
        result.errorMessage =
            QStringLiteral("Failed to start temporary Xray: %1").arg(xrayProcess.errorString());
        QFile::remove(configPath);
        return result;
    }

    const int proxyReadyTimeoutMs = qMin(5000, timeoutMs);
    if (!waitForLocalPort(ports.httpPort, proxyReadyTimeoutMs)) {
        stopXray();
        QFile::remove(configPath);
        result.status = TestStatus::Timeout;
        result.errorMessage =
            QStringLiteral("Local HTTP proxy did not become ready in time.");
        logLine(log, result.errorMessage);
        return result;
    }
    logLine(log, QStringLiteral("Local HTTP proxy is ready"));

    QNetworkAccessManager manager;
    QNetworkProxy proxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), ports.httpPort);
    manager.setProxy(proxy);

    const QUrl url(testUrl);
    QNetworkRequest httpRequest;
    httpRequest.setUrl(url);
    httpRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);

    logLine(log, QStringLiteral("Requesting %1").arg(testUrl));

    QElapsedTimer timer;
    timer.start();
    QNetworkReply* reply = manager.get(httpRequest);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeoutTimer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        stopXray();
        QFile::remove(configPath);
        result.status = TestStatus::Timeout;
        result.errorMessage = QStringLiteral("HTTP request through proxy timed out.");
        logLine(log, result.errorMessage);
        reply->deleteLater();
        return result;
    }

    const int elapsedMs = static_cast<int>(timer.elapsed());
    const int httpStatus =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        result.status = TestStatus::Failed;
        result.errorMessage = reply->errorString();
        logLine(log, QStringLiteral("Test failed: %1").arg(result.errorMessage));
        reply->deleteLater();
        stopXray();
        QFile::remove(configPath);
        return result;
    }

    if (!isSuccessHttpStatus(httpStatus)) {
        result.status = TestStatus::Failed;
        result.errorMessage =
            QStringLiteral("Unexpected HTTP status: %1").arg(httpStatus);
        logLine(log, QStringLiteral("Test failed: %1").arg(result.errorMessage));
        reply->deleteLater();
        stopXray();
        QFile::remove(configPath);
        return result;
    }

    result.realDelayMs = elapsedMs;
    result.status = TestStatus::Available;
    logLine(log, QStringLiteral("Real delay OK: %1 ms").arg(elapsedMs));

    reply->deleteLater();
    stopXray();
    logLine(log, QStringLiteral("Temporary Xray stopped"));
    QFile::remove(configPath);
    return result;
}

} // namespace zarya
