#include <QCoreApplication>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTcpServer>

#include <cstdio>
#include <cstddef>

namespace {

int fail(const char* message, const QByteArray& output = {})
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    if (!output.isEmpty()) {
        std::fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stderr);
        std::fputc('\n', stderr);
    }
    return 1;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 2) {
        return fail("worker path argument is required");
    }

    QTcpServer portReservation;
    if (!portReservation.listen(QHostAddress::LocalHost, 0)) {
        return fail("could not allocate a local proxy port");
    }
    const int proxyPort = portReservation.serverPort();
    portReservation.close();

    const QJsonObject config{
        {QStringLiteral("log"), QJsonObject{
             {QStringLiteral("loglevel"), QStringLiteral("warning")},
         }},
        {QStringLiteral("inbounds"), QJsonArray{QJsonObject{
             {QStringLiteral("listen"), QStringLiteral("127.0.0.1")},
             {QStringLiteral("port"), proxyPort},
             {QStringLiteral("protocol"), QStringLiteral("mixed")},
             {QStringLiteral("tag"), QStringLiteral("mixed-in")},
             {QStringLiteral("settings"), QJsonObject{{QStringLiteral("udp"), true}}},
         }}},
        {QStringLiteral("outbounds"), QJsonArray{QJsonObject{
             {QStringLiteral("tag"), QStringLiteral("proxy")},
             {QStringLiteral("protocol"), QStringLiteral("freedom")},
         }}},
    };
    const QString workerPath = QString::fromLocal8Bit(argv[1]);
    const QJsonObject request{
        {QStringLiteral("operation"), QStringLiteral("realDelay")},
        {QStringLiteral("config"), config},
        {QStringLiteral("assetDir"), QFileInfo(workerPath).absolutePath()},
        {QStringLiteral("dataDir"), QFileInfo(workerPath).absolutePath()},
        {QStringLiteral("proxyPort"), proxyPort},
        {QStringLiteral("url"), QStringLiteral("https://127.0.0.1:1/")},
        {QStringLiteral("timeoutMs"), 2000},
    };

    QProcess worker;
    worker.setProgram(workerPath);
    worker.start();
    if (!worker.waitForStarted(5000)) {
        return fail("worker did not start", worker.readAllStandardError());
    }
    worker.write(QJsonDocument(request).toJson(QJsonDocument::Compact));
    worker.closeWriteChannel();
    if (!worker.waitForFinished(15000)) {
        worker.kill();
        worker.waitForFinished(3000);
        return fail("worker timed out");
    }

    const QByteArray output = worker.readAllStandardOutput().trimmed();
    QJsonParseError parseError;
    const QJsonDocument response = QJsonDocument::fromJson(output, &parseError);
    if (!response.isObject()) {
        return fail("worker stdout is not exactly one JSON object", output);
    }
    if (!response.object().contains(QStringLiteral("success"))) {
        return fail("worker response has no success field", output);
    }
    std::fprintf(stdout, "PASS: worker stdout is exactly one JSON object\n");
    return 0;
}
