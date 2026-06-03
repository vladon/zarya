#include "domain/Profile.h"
#include "testing/PortAllocator.h"
#include "testing/TcpPingTester.h"
#include "testing/TestResult.h"
#include "testing/TestStatus.h"

#include <QCoreApplication>

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
    if (ports.socksPort <= 0 || ports.httpPort <= 0 || ports.socksPort == ports.httpPort) {
        return fail("allocate inbound ports");
    }
    pass("allocate inbound ports");

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
