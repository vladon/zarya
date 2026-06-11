#include "updater/AppVersion.h"

#include <QCoreApplication>

#include <cstdlib>

namespace {

int g_failures = 0;

void expectTrue(bool condition, const char* message)
{
    if (!condition) {
        ++g_failures;
        qWarning("FAIL: %s", message);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    expectTrue(zarya::AppVersion::isLessThan(QStringLiteral("0.9.0"), QStringLiteral("0.10.0")),
               "0.9.0 < 0.10.0");
    expectTrue(zarya::AppVersion::isLessThan(QStringLiteral("0.32.0-beta"),
                                             QStringLiteral("0.33.0-beta")),
               "0.32.0-beta < 0.33.0-beta");
    expectTrue(zarya::AppVersion::isGreaterThan(QStringLiteral("1.0.0"),
                                                QStringLiteral("0.99.0-beta")),
               "1.0.0 > 0.99.0-beta");
    expectTrue(zarya::AppVersion::isLessThan(QStringLiteral("0.33.0-dev"),
                                             QStringLiteral("0.33.0-beta")),
               "0.33.0-dev < 0.33.0-beta");
    expectTrue(zarya::AppVersion::isLessThan(QStringLiteral("0.33.0-beta"),
                                             QStringLiteral("1.0.0")),
               "0.33.0-beta < 1.0.0");

    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
