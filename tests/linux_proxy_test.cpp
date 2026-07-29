#include "platform/PlatformProcessUtils.h"
#include "platform/linux/GnomeSystemProxyManager.h"
#include "platform/linux/KdeSystemProxyManager.h"
#include "platform/linux/LinuxDesktopEnvironment.h"
#include "platform/linux/LinuxSystemProxyManager.h"

#include <QCoreApplication>
#include <QMap>
#include <QProcessEnvironment>

#include <algorithm>
#include <cstdio>

namespace {

int g_failures = 0;

void expectTrue(bool condition, const char* message)
{
    if (condition) {
        std::fprintf(stdout, "PASS: %s\n", message);
        return;
    }
    ++g_failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
}

void expectEqual(const QString& actual, const QString& expected, const char* message)
{
    if (actual == expected) {
        std::fprintf(stdout, "PASS: %s\n", message);
        return;
    }
    ++g_failures;
    std::fprintf(stderr, "FAIL: %s\n  expected: %s\n  actual: %s\n", message,
                 expected.toUtf8().constData(), actual.toUtf8().constData());
}

struct RecordedCommand {
    QString program;
    QStringList arguments;
    int timeoutMs = 0;
};

class FakeGsettings {
public:
    bool available = true;
    QString failSetKey;
    QMap<QString, QString> values;
    QList<RecordedCommand> commands;

    zarya::PlatformProcessRunner runner()
    {
        return [this](const QString& program, const QStringList& arguments, int timeoutMs) {
            commands.append({program, arguments, timeoutMs});

            zarya::ProcessResult result;
            result.exitCode = 0;

            if (program != QStringLiteral("gsettings")) {
                result.errorMessage = QStringLiteral("Unexpected program");
                return result;
            }
            if (arguments == QStringList{QStringLiteral("help")}) {
                result.success = available;
                if (!available) {
                    result.exitCode = 1;
                    result.errorMessage = QStringLiteral("gsettings unavailable");
                }
                return result;
            }
            if (arguments.size() == 2 && arguments.at(0) == QStringLiteral("get")) {
                result.success = values.contains(arguments.at(1));
                if (result.success) {
                    result.standardOutput = values.value(arguments.at(1));
                } else {
                    result.exitCode = 1;
                    result.errorMessage = QStringLiteral("Unknown key");
                }
                return result;
            }
            if (arguments.size() == 3 && arguments.at(0) == QStringLiteral("set")) {
                if (arguments.at(1) == failSetKey) {
                    result.exitCode = 1;
                    result.errorMessage = QStringLiteral("Injected set failure");
                    return result;
                }
                values.insert(arguments.at(1), arguments.at(2));
                result.success = true;
                return result;
            }

            result.exitCode = 1;
            result.errorMessage = QStringLiteral("Unexpected arguments");
            return result;
        };
    }
};

QMap<QString, QString> initialGsettings()
{
    return {
        {QStringLiteral("org.gnome.system.proxy.mode"), QStringLiteral("'auto'")},
        {QStringLiteral("org.gnome.system.proxy.ignore-hosts"),
         QStringLiteral("['localhost', 'intranet.example']")},
        {QStringLiteral("org.gnome.system.proxy.use-same-proxy"), QStringLiteral("true")},
        {QStringLiteral("org.gnome.system.proxy.autoconfig-url"),
         QStringLiteral("'https://config.example/proxy.pac'")},
        {QStringLiteral("org.gnome.system.proxy.http.host"),
         QStringLiteral("'old-http.example'")},
        {QStringLiteral("org.gnome.system.proxy.http.port"), QStringLiteral("8080")},
        {QStringLiteral("org.gnome.system.proxy.https.host"),
         QStringLiteral("'old-https.example'")},
        {QStringLiteral("org.gnome.system.proxy.https.port"), QStringLiteral("8443")},
        {QStringLiteral("org.gnome.system.proxy.socks.host"),
         QStringLiteral("'old-socks.example'")},
        {QStringLiteral("org.gnome.system.proxy.socks.port"), QStringLiteral("1080")},
    };
}

void testDesktopDetection()
{
    using zarya::LinuxDesktopEnvironment;
    using zarya::LinuxDesktopEnvironmentDetector;

    QProcessEnvironment environment;
    expectTrue(LinuxDesktopEnvironmentDetector::detect(environment)
                   == LinuxDesktopEnvironment::Unknown,
               "empty environment selects unknown desktop");

    environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"),
                       QStringLiteral("ubuntu:GNOME"));
    expectTrue(LinuxDesktopEnvironmentDetector::detect(environment)
                   == LinuxDesktopEnvironment::Gnome,
               "XDG_CURRENT_DESKTOP selects GNOME");

    environment = QProcessEnvironment();
    environment.insert(QStringLiteral("DESKTOP_SESSION"), QStringLiteral("plasmawayland"));
    expectTrue(LinuxDesktopEnvironmentDetector::detect(environment)
                   == LinuxDesktopEnvironment::Kde,
               "DESKTOP_SESSION selects KDE Plasma");

    environment.insert(QStringLiteral("GNOME_DESKTOP_SESSION_ID"), QStringLiteral("legacy"));
    expectTrue(LinuxDesktopEnvironmentDetector::detect(environment)
                   == LinuxDesktopEnvironment::Kde,
               "KDE signal wins when desktop variables conflict");

    expectEqual(LinuxDesktopEnvironmentDetector::displayName(LinuxDesktopEnvironment::Gnome),
                QStringLiteral("GNOME"), "GNOME display name");
    expectEqual(LinuxDesktopEnvironmentDetector::displayName(LinuxDesktopEnvironment::Kde),
                QStringLiteral("KDE/Plasma"), "KDE display name");
    expectEqual(LinuxDesktopEnvironmentDetector::displayName(LinuxDesktopEnvironment::Unknown),
                QStringLiteral("Unknown"), "unknown desktop display name");
}

void testLinuxBackendSelection()
{
    FakeGsettings available;
    zarya::LinuxSystemProxyManager gnome(zarya::LinuxDesktopEnvironment::Gnome,
                                         available.runner());
    expectTrue(gnome.isSupported(), "available gsettings enables GNOME backend");
    expectEqual(gnome.backendName(), QStringLiteral("GNOME gsettings"),
                "GNOME backend selected");
    expectEqual(gnome.detectedDesktopName(), QStringLiteral("GNOME"),
                "GNOME desktop name retained");

    FakeGsettings missing;
    missing.available = false;
    zarya::LinuxSystemProxyManager unsupportedGnome(
        zarya::LinuxDesktopEnvironment::Gnome, missing.runner());
    expectTrue(!unsupportedGnome.isSupported(), "missing gsettings disables GNOME backend");
    expectEqual(unsupportedGnome.supportLevel(), QStringLiteral("unsupported"),
                "missing gsettings reports unsupported");
    expectTrue(unsupportedGnome.limitations().contains(QStringLiteral("not available")),
               "missing gsettings reports actionable limitation");

    zarya::LinuxSystemProxyManager kde(zarya::LinuxDesktopEnvironment::Kde,
                                       available.runner());
    expectTrue(!kde.isSupported(), "KDE placeholder remains unsupported");
    expectEqual(kde.supportLevel(), QStringLiteral("partial"),
                "KDE placeholder remains partial");
    expectEqual(kde.detectedDesktopName(), QStringLiteral("KDE/Plasma"),
                "KDE desktop name retained");

    zarya::LinuxSystemProxyManager unknown(zarya::LinuxDesktopEnvironment::Unknown,
                                           available.runner());
    expectTrue(!unknown.isSupported(), "unknown desktop remains unsupported");
    expectTrue(unknown.limitations().contains(QStringLiteral("not supported")),
               "unknown desktop reports limitation");
}

void testGnomeSnapshotApplyRestore()
{
    FakeGsettings fake;
    fake.values = initialGsettings();
    const QMap<QString, QString> before = fake.values;
    zarya::GnomeSystemProxyManager manager(fake.runner());

    QString error;
    const zarya::SystemProxyState snapshot = manager.readCurrentState(&error);
    expectTrue(error.isEmpty(), "GNOME snapshot succeeds");
    expectTrue(snapshot.proxyEnabled == false, "automatic mode is not reported as manual proxy");
    expectEqual(snapshot.proxyServer, QStringLiteral("old-http.example:8080"),
                "GNOME snapshot exposes prior HTTP endpoint");
    expectTrue(snapshot.rawValues.value(QStringLiteral("gsettings")).toMap().size() == 10,
               "GNOME snapshot preserves every overwritten key");

    expectTrue(manager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "GNOME proxy apply succeeds");
    expectTrue(error.isEmpty(), "GNOME proxy apply has no error");
    expectEqual(fake.values.value(QStringLiteral("org.gnome.system.proxy.mode")),
                QStringLiteral("'manual'"), "GNOME mode changed to manual");
    expectEqual(fake.values.value(QStringLiteral("org.gnome.system.proxy.http.host")),
                QStringLiteral("'127.0.0.1'"), "GNOME HTTP host uses loopback");
    expectEqual(fake.values.value(QStringLiteral("org.gnome.system.proxy.https.port")),
                QStringLiteral("10808"), "GNOME HTTPS port uses mixed inbound");

    expectTrue(manager.restoreState(snapshot, &error), "GNOME proxy restore succeeds");
    expectTrue(error.isEmpty(), "GNOME proxy restore has no error");
    expectTrue(fake.values == before, "GNOME restore reproduces the complete snapshot");

    const bool onlyGsettings = std::all_of(
        fake.commands.cbegin(), fake.commands.cend(), [](const RecordedCommand& command) {
            return command.program == QStringLiteral("gsettings");
        });
    expectTrue(onlyGsettings, "GNOME runner never invokes a shell");

    const bool boundedTimeouts = std::all_of(
        fake.commands.cbegin(), fake.commands.cend(), [](const RecordedCommand& command) {
            return command.timeoutMs == 5000;
        });
    expectTrue(boundedTimeouts, "GNOME runner uses bounded timeouts");
}

void testGnomeFailures()
{
    FakeGsettings fake;
    fake.values = initialGsettings();
    zarya::GnomeSystemProxyManager manager(fake.runner());

    QString error;
    const qsizetype commandCount = fake.commands.size();
    expectTrue(!manager.applyHttpProxy(QStringLiteral("127.0.0.1"), 0, &error),
               "invalid GNOME proxy port is rejected");
    expectTrue(error.contains(QStringLiteral("Invalid HTTP proxy port")),
               "invalid GNOME proxy port has actionable error");
    expectTrue(fake.commands.size() == commandCount,
               "invalid GNOME proxy port executes no command");

    fake.failSetKey = QStringLiteral("org.gnome.system.proxy.http.host");
    error.clear();
    expectTrue(!manager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "injected gsettings failure is propagated");
    expectTrue(error.contains(QStringLiteral("Injected set failure")),
               "gsettings failure remains actionable");

    zarya::SystemProxyState emptySnapshot;
    error.clear();
    expectTrue(!manager.restoreState(emptySnapshot, &error),
               "missing GNOME snapshot is rejected");
    expectTrue(error.contains(QStringLiteral("Missing GNOME gsettings snapshot")),
               "missing GNOME snapshot has actionable error");
}

void testKdePlaceholder()
{
    zarya::KdeSystemProxyManager manager;
    QString error;
    expectTrue(!manager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "KDE placeholder does not modify desktop settings");
    expectTrue(error.contains(QStringLiteral("does not modify KDE proxy settings")),
               "KDE placeholder explains its limitation");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    testDesktopDetection();
    testLinuxBackendSelection();
    testGnomeSnapshotApplyRestore();
    testGnomeFailures();
    testKdePlaceholder();

    if (g_failures == 0) {
        std::fprintf(stdout, "All Linux proxy tests passed.\n");
    }
    return g_failures == 0 ? 0 : 1;
}
