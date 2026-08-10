#include "domain/Profile.h"
#include "testing/CoreTestWorkerProtocol.h"
#include "testing/PortAllocator.h"
#include "testing/TcpPingTester.h"
#include "testing/TestResult.h"
#include "testing/TestStatus.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

#include <cstdio>

namespace zarya {
namespace {

bool fail(const char* message)
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    return false;
}

bool pass(const char* message)
{
    std::fprintf(stdout, "PASS: %s\n", message);
    return true;
}

int runAll()
{
    if (testStatusFromString(testStatusToString(TestStatus::Available)) != TestStatus::Available) {
        return fail("test status round trip");
    }
    pass("test status round trip");

    if (testStatusDisplayString(TestStatus::Unsupported) != QStringLiteral("Unsupported")) {
        return fail("test status display string");
    }
    pass("test status display string");

    const int port = PortAllocator::allocateFreeLocalPort();
    if (port <= 0) {
        return fail("allocate free local port");
    }
    pass("allocate free local port");

    const InboundPorts ports = PortAllocator::allocateInboundPorts();
    if (ports.mixedPort <= 0) {
        return fail("allocate inbound ports");
    }
    pass("allocate inbound ports");

    const QJsonObject isolatedConfig = prepareIsolatedXrayTestConfig({
        {QStringLiteral("log"), QJsonObject{
             {QStringLiteral("loglevel"), QStringLiteral("warning")},
             {QStringLiteral("access"), QStringLiteral("stdout")},
         }},
        {QStringLiteral("inbounds"), QJsonArray{}},
    });
    const QJsonObject isolatedLog = isolatedConfig.value(QStringLiteral("log")).toObject();
    if (isolatedLog.value(QStringLiteral("loglevel")).toString()
            != QStringLiteral("none")
        || isolatedLog.contains(QStringLiteral("access"))) {
        return fail("isolated worker disables Xray console logging");
    }
    pass("isolated worker disables Xray console logging");

    QJsonObject workerResponse;
    QString workerResponseError;
    const QByteArray prefixedResponse =
        "2026/08/10 [Warning] core: Xray started\n"
        "{\"delayMs\":123,\"success\":true}\n";
    if (!parseCoreTestWorkerResponse(
            prefixedResponse, &workerResponse, &workerResponseError)
        || !workerResponse.value(QStringLiteral("success")).toBool()
        || workerResponse.value(QStringLiteral("delayMs")).toInt() != 123) {
        return fail("worker response parser ignores non-protocol log prefix");
    }
    pass("worker response parser ignores non-protocol log prefix");

    if (parseCoreTestWorkerResponse(
            QByteArrayLiteral("{\"diagnostic\":true}\n"),
            &workerResponse,
            &workerResponseError)) {
        return fail("worker response parser requires success field");
    }
    pass("worker response parser requires success field");

    if (parseCoreTestWorkerResponse(
            QByteArrayLiteral("not json\n"), &workerResponse, &workerResponseError)
        || workerResponseError.isEmpty()) {
        return fail("worker response parser rejects missing JSON object");
    }
    pass("worker response parser rejects missing JSON object");

    Profile invalid = Profile::createVlessRealityDefault();
    invalid.address.clear();
    const TestResult invalidResult = TcpPingTester::run(invalid, 1000);
    if (invalidResult.status != TestStatus::Failed) {
        return fail("tcp ping invalid address");
    }
    pass("tcp ping invalid address");

    Profile unreachable = Profile::createVlessRealityDefault();
    unreachable.address = QStringLiteral("10.255.255.1");
    unreachable.port = 1;
    const TestResult unreachableResult = TcpPingTester::run(unreachable, 500);
    if (unreachableResult.status != TestStatus::Timeout
        && unreachableResult.status != TestStatus::Failed) {
        return fail("tcp ping unreachable host");
    }
    pass("tcp ping unreachable host");

    std::fprintf(stdout, "All testing tests passed.\n");
    return 0;
}

} // namespace
} // namespace zarya

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    return zarya::runAll();
}
