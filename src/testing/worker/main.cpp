#include "runtime/core/CoreRuntimeCoordinator.h"
#include "runtime/embedded/xray/EmbeddedXrayRuntimeHost.h"
#include "testing/CoreTestWorkerProtocol.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QThread>
#include <QNetworkRequest>
#include <QTimer>

#include <cstdio>

namespace {

void writeResult(const QJsonObject& result)
{
    const QByteArray line = QJsonDocument(result).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(line.constData(), 1, static_cast<std::size_t>(line.size()), stdout);
    std::fflush(stdout);
}

QJsonObject failure(const QString& code, const QString& message)
{
    return {{QStringLiteral("success"), false},
            {QStringLiteral("errorCode"), code},
            {QStringLiteral("message"), message}};
}

bool waitForLocalPort(int port, int timeoutMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < timeoutMs) {
        QTcpSocket socket;
        socket.connectToHost(QStringLiteral("127.0.0.1"), static_cast<quint16>(port));
        if (socket.waitForConnected(150)) {
            socket.disconnectFromHost();
            return true;
        }
        QThread::msleep(50);
    }
    return false;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);

    QFile inputFile;
    if (!inputFile.open(stdin, QIODevice::ReadOnly)) {
        writeResult(failure(QStringLiteral("worker.stdin.failed"), inputFile.errorString()));
        return 2;
    }
    const QByteArray input = inputFile.readAll();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(input, &parseError);
    if (document.isNull() || !document.isObject()) {
        writeResult(failure(QStringLiteral("worker.request.invalid"), parseError.errorString()));
        return 2;
    }

    const QJsonObject object = document.object();
    const QString operation = object.value(QStringLiteral("operation")).toString();
    const QJsonValue configValue = object.value(QStringLiteral("config"));
    if (!configValue.isObject()) {
        writeResult(failure(QStringLiteral("worker.config.invalid"),
                            QStringLiteral("Request config must be a JSON object.")));
        return 2;
    }

    zarya::CoreRuntimeCoordinator coordinator;
    zarya::EmbeddedXrayRuntimeHost host(&coordinator);
    if (!host.isAvailable()) {
        writeResult(failure(QStringLiteral("worker.xray.unavailable"), host.loadStatus()));
        return 3;
    }

    zarya::CoreLaunchRequest request;
    request.coreType = zarya::CoreType::Xray;
    request.configJson = QJsonDocument(
        zarya::prepareIsolatedXrayTestConfig(configValue.toObject()))
                             .toJson(QJsonDocument::Compact);
    request.assetDir = object.value(QStringLiteral("assetDir")).toString();
    request.dataDir = object.value(QStringLiteral("dataDir")).toString();

    if (operation == QStringLiteral("validate")) {
        const zarya::CoreOperationResult result = host.validate(request);
        writeResult({{QStringLiteral("success"), result.success},
                     {QStringLiteral("errorCode"), result.errorCode},
                     {QStringLiteral("message"), result.message},
                     {QStringLiteral("version"), host.version()},
                     {QStringLiteral("abiVersion"), host.abiVersion()}});
        return result.success ? 0 : 4;
    }

    if (operation != QStringLiteral("realDelay")) {
        writeResult(failure(QStringLiteral("worker.operation.unsupported"),
                            QStringLiteral("Unsupported worker operation.")));
        return 2;
    }

    const zarya::CoreOperationResult started = host.start(request);
    if (!started.success) {
        writeResult(failure(started.errorCode, started.message));
        return 5;
    }

    const int proxyPort = object.value(QStringLiteral("proxyPort")).toInt();
    const int timeoutMs = qBound(1000,
                                 object.value(QStringLiteral("timeoutMs")).toInt(15000),
                                 60000);
    const QUrl testUrl(object.value(QStringLiteral("url")).toString());
    if (proxyPort <= 0 || !testUrl.isValid() || testUrl.scheme() != QStringLiteral("https")) {
        host.stop();
        writeResult(failure(QStringLiteral("worker.probe.invalid"),
                            QStringLiteral("An HTTPS URL and proxyPort are required.")));
        return 2;
    }

    if (!waitForLocalPort(proxyPort, qMin(timeoutMs, 5000))) {
        host.stop();
        writeResult(failure(QStringLiteral("worker.ready.timeout"),
                            QStringLiteral("Embedded Xray mixed proxy did not become ready.")));
        return 6;
    }

    QNetworkAccessManager network;
    network.setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"),
                                   static_cast<quint16>(proxyPort)));
    QElapsedTimer elapsed;
    elapsed.start();
    QNetworkReply* reply = network.get(QNetworkRequest(testUrl));
    QTimer timeout;
    bool timedOut = false;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, reply, [&]() {
        timedOut = true;
        reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &application, [&]() {
        const int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool acceptedStatus = statusCode == 200 || statusCode == 204
                                    || statusCode == 301 || statusCode == 302;
        const bool success = !timedOut && reply->error() == QNetworkReply::NoError
                             && acceptedStatus;
        const QString message = timedOut
            ? QStringLiteral("HTTP request through embedded Xray timed out.")
            : (success ? QString() : reply->errorString());
        host.stop();
        writeResult({{QStringLiteral("success"), success},
                     {QStringLiteral("errorCode"),
                      success ? QString()
                              : (timedOut ? QStringLiteral("worker.probe.timeout")
                                          : QStringLiteral("worker.probe.failed"))},
                     {QStringLiteral("message"), message},
                     {QStringLiteral("delayMs"), static_cast<double>(elapsed.elapsed())}});
        reply->deleteLater();
        application.exit(success ? 0 : 6);
    });
    timeout.start(timeoutMs);
    return application.exec();
}
