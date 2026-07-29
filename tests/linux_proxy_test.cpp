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

class FakeKdeConfig {
public:
    bool version6Available = true;
    bool version5Available = true;
    bool sessionBusAvailable = true;
    QString failWriteKey;
    int writeFailuresRemaining = 0;
    int reloadFailuresRemaining = 0;
    int reloadCount = 0;
    QMap<QString, QString> values;
    QList<RecordedCommand> commands;

    zarya::PlatformProcessRunner runner()
    {
        return [this](const QString& program, const QStringList& arguments, int timeoutMs) {
            commands.append({program, arguments, timeoutMs});

            zarya::ProcessResult result;
            result.exitCode = 0;

            const bool readTool =
                program == QStringLiteral("kreadconfig6")
                || program == QStringLiteral("kreadconfig5");
            const bool writeTool =
                program == QStringLiteral("kwriteconfig6")
                || program == QStringLiteral("kwriteconfig5");
            const bool versionAvailable =
                program.endsWith(QLatin1Char('6')) ? version6Available : version5Available;

            if ((readTool || writeTool)
                && arguments == QStringList{QStringLiteral("--help")}) {
                result.success = versionAvailable;
                if (!versionAvailable) {
                    result.exitCode = 1;
                    result.errorMessage = QStringLiteral("KConfig tool unavailable");
                }
                return result;
            }

            const qsizetype keyIndex = arguments.indexOf(QStringLiteral("--key"));
            if (readTool && versionAvailable && keyIndex >= 0
                && keyIndex + 1 < arguments.size()) {
                const QString key = arguments.at(keyIndex + 1);
                result.success = true;
                result.standardOutput =
                    values.contains(key)
                        ? values.value(key) + QLatin1Char('\n')
                        : QStringLiteral("__zarya_kde_proxy_key_missing_5e83f262__\n");
                return result;
            }
            if (writeTool && versionAvailable && keyIndex >= 0
                && keyIndex + 1 < arguments.size()) {
                const QString key = arguments.at(keyIndex + 1);
                if (key == failWriteKey && writeFailuresRemaining > 0) {
                    --writeFailuresRemaining;
                    result.exitCode = 1;
                    result.errorMessage = QStringLiteral("Injected KDE write failure");
                    return result;
                }
                if (arguments.last() == QStringLiteral("--delete")) {
                    values.remove(key);
                } else {
                    values.insert(key, arguments.last());
                }
                result.success = true;
                return result;
            }
            if (program == QStringLiteral("dbus-send")) {
                if (arguments.contains(QStringLiteral("org.freedesktop.DBus.ListNames"))) {
                    result.success = sessionBusAvailable;
                    if (!sessionBusAvailable) {
                        result.exitCode = 1;
                        result.errorMessage = QStringLiteral("Session bus unavailable");
                    }
                    return result;
                }
                ++reloadCount;
                if (reloadFailuresRemaining > 0) {
                    --reloadFailuresRemaining;
                    result.exitCode = 1;
                    result.errorMessage = QStringLiteral("Injected KDE reload failure");
                    return result;
                }
                result.success = true;
                return result;
            }

            result.exitCode = 1;
            result.errorMessage = QStringLiteral("Unexpected KDE command");
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

QMap<QString, QString> initialKdeConfig()
{
    return {
        {QStringLiteral("ProxyType"), QStringLiteral("2")},
        {QStringLiteral("httpProxy"), QStringLiteral("http://old-http.example:8080")},
        {QStringLiteral("NoProxyFor"), QStringLiteral("intranet.example,localhost")},
        {QStringLiteral("Proxy Config Script"),
         QStringLiteral("https://config.example/proxy.pac")},
        {QStringLiteral("ReversedException"), QStringLiteral("true")},
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

    FakeKdeConfig kdeConfig;
    zarya::LinuxSystemProxyManager kde(zarya::LinuxDesktopEnvironment::Kde,
                                       kdeConfig.runner());
    expectTrue(kde.isSupported(), "KDE config tools enable KDE backend");
    expectEqual(kde.supportLevel(), QStringLiteral("full"),
                "KDE backend reports full integration");
    expectEqual(kde.backendName(), QStringLiteral("KDE/Plasma kioslaverc"),
                "KDE backend selected");
    expectEqual(kde.detectedDesktopName(), QStringLiteral("KDE/Plasma"),
                "KDE desktop name retained");

    kdeConfig.version6Available = false;
    kdeConfig.version5Available = false;
    zarya::LinuxSystemProxyManager unsupportedKde(
        zarya::LinuxDesktopEnvironment::Kde, kdeConfig.runner());
    expectTrue(!unsupportedKde.isSupported(), "missing KConfig tools disables KDE backend");
    expectEqual(unsupportedKde.supportLevel(), QStringLiteral("unsupported"),
                "missing KConfig tools reports unsupported");

    FakeKdeConfig noSessionBus;
    noSessionBus.sessionBusAvailable = false;
    zarya::LinuxSystemProxyManager unsupportedKdeBus(
        zarya::LinuxDesktopEnvironment::Kde, noSessionBus.runner());
    expectTrue(!unsupportedKdeBus.isSupported(),
               "missing KDE session bus disables KDE backend");

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

void testKdeSnapshotApplyRestore()
{
    FakeKdeConfig fake;
    fake.values = initialKdeConfig();
    const QMap<QString, QString> before = fake.values;
    zarya::KdeSystemProxyManager manager(fake.runner());

    QString error;
    const zarya::SystemProxyState snapshot = manager.readCurrentState(&error);
    expectTrue(error.isEmpty(), "KDE snapshot succeeds");
    expectTrue(!snapshot.proxyEnabled, "KDE PAC mode is not reported as manual proxy");
    expectEqual(snapshot.proxyServer, QStringLiteral("old-http.example:8080"),
                "KDE snapshot exposes prior HTTP endpoint");
    const QVariantMap raw = snapshot.rawValues.value(QStringLiteral("kioslaverc")).toMap();
    expectTrue(raw.size() == 6, "KDE snapshot preserves proxy and PAC keys");
    expectTrue(!raw.value(QStringLiteral("httpsProxy")).toMap()
                    .value(QStringLiteral("present")).toBool(),
               "KDE snapshot preserves a missing key");

    expectTrue(manager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "KDE proxy apply succeeds");
    expectTrue(error.isEmpty(), "KDE proxy apply has no error");
    expectEqual(fake.values.value(QStringLiteral("ProxyType")), QStringLiteral("1"),
                "KDE mode changed to manual");
    expectEqual(fake.values.value(QStringLiteral("httpProxy")),
                QStringLiteral("http://127.0.0.1:10808"),
                "KDE HTTP proxy uses loopback mixed inbound");
    expectEqual(fake.values.value(QStringLiteral("httpsProxy")),
                QStringLiteral("http://127.0.0.1:10808"),
                "KDE HTTPS proxy uses loopback mixed inbound");
    expectEqual(fake.values.value(QStringLiteral("ReversedException")),
                QStringLiteral("false"), "KDE exclusions are not reversed");
    expectTrue(fake.reloadCount == 1, "KDE apply reloads running KIO applications");

    zarya::KdeSystemProxyManager restartedManager(fake.runner());
    expectTrue(restartedManager.restoreState(snapshot, &error),
               "KDE proxy restore succeeds after manager restart");
    expectTrue(error.isEmpty(), "KDE proxy restore has no error");
    expectTrue(fake.values == before,
               "KDE restore reproduces values and removes originally missing keys");
    expectTrue(fake.reloadCount == 2, "KDE restore reloads running KIO applications");

    expectTrue(restartedManager.restoreState(snapshot, &error),
               "restoring the same KDE snapshot twice is safe");
    expectTrue(fake.values == before, "second KDE restore remains exact");

    const bool noShell = std::all_of(
        fake.commands.cbegin(), fake.commands.cend(), [](const RecordedCommand& command) {
            return command.program != QStringLiteral("sh")
                && command.program != QStringLiteral("bash");
        });
    expectTrue(noShell, "KDE runner never invokes a shell");

    const bool boundedTimeouts = std::all_of(
        fake.commands.cbegin(), fake.commands.cend(), [](const RecordedCommand& command) {
            return command.timeoutMs == 5000;
        });
    expectTrue(boundedTimeouts, "KDE runner uses bounded timeouts");
}

void testKdeFallbackAndFailures()
{
    FakeKdeConfig fallback;
    fallback.version6Available = false;
    fallback.values = initialKdeConfig();
    zarya::KdeSystemProxyManager fallbackManager(fallback.runner());
    expectTrue(fallbackManager.isSupported(), "KDE backend falls back to KConfig 5 tools");
    QString error;
    expectTrue(fallbackManager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "KConfig 5 fallback applies proxy");
    const bool usedVersion5 = std::any_of(
        fallback.commands.cbegin(), fallback.commands.cend(),
        [](const RecordedCommand& command) {
            return command.program == QStringLiteral("kwriteconfig5");
        });
    expectTrue(usedVersion5, "KDE fallback writes with kwriteconfig5");

    FakeKdeConfig modes;
    zarya::KdeSystemProxyManager modesManager(modes.runner());
    modes.values.insert(QStringLiteral("ProxyType"), QStringLiteral("0"));
    zarya::SystemProxyState modeSnapshot = modesManager.readCurrentState(&error);
    expectTrue(!modeSnapshot.proxyEnabled, "KDE no-proxy mode is reported as disabled");
    modes.values.insert(QStringLiteral("ProxyType"), QStringLiteral("1"));
    modes.values.insert(QStringLiteral("httpProxy"),
                        QStringLiteral("http://manual.example:3128"));
    modeSnapshot = modesManager.readCurrentState(&error);
    expectTrue(modeSnapshot.proxyEnabled, "KDE manual mode is reported as enabled");
    modes.values.insert(QStringLiteral("ProxyType"), QStringLiteral("3"));
    modeSnapshot = modesManager.readCurrentState(&error);
    expectTrue(!modeSnapshot.proxyEnabled,
               "KDE automatic discovery mode is not reported as manual");

    const QMap<QString, QString> beforeRepeatedApply = modes.values;
    expectTrue(modesManager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "first repeated KDE apply succeeds");
    expectTrue(modesManager.applyHttpProxy(QStringLiteral("127.0.0.1"), 10808, &error),
               "applying the same KDE proxy twice is safe");
    expectEqual(modes.values.value(QStringLiteral("ProxyType")), QStringLiteral("1"),
                "repeated KDE apply remains in manual mode");
    expectTrue(modes.values != beforeRepeatedApply,
               "repeated KDE apply keeps the requested proxy active");

    FakeKdeConfig writeFailure;
    writeFailure.values = initialKdeConfig();
    const QMap<QString, QString> beforeWriteFailure = writeFailure.values;
    writeFailure.failWriteKey = QStringLiteral("NoProxyFor");
    writeFailure.writeFailuresRemaining = 1;
    zarya::KdeSystemProxyManager writeFailureManager(writeFailure.runner());
    error.clear();
    expectTrue(!writeFailureManager.applyHttpProxy(
                   QStringLiteral("127.0.0.1"), 10808, &error),
               "partial KDE write failure is propagated");
    expectTrue(error.contains(QStringLiteral("Previous KDE proxy settings were restored")),
               "partial KDE write failure reports successful rollback");
    expectTrue(writeFailure.values == beforeWriteFailure,
               "partial KDE write failure rolls back all changed keys");

    FakeKdeConfig reloadFailure;
    reloadFailure.values = initialKdeConfig();
    const QMap<QString, QString> beforeReloadFailure = reloadFailure.values;
    reloadFailure.reloadFailuresRemaining = 1;
    zarya::KdeSystemProxyManager reloadFailureManager(reloadFailure.runner());
    error.clear();
    expectTrue(!reloadFailureManager.applyHttpProxy(
                   QStringLiteral("127.0.0.1"), 10808, &error),
               "KDE reload failure is propagated");
    expectTrue(error.contains(QStringLiteral("Previous KDE proxy settings were restored")),
               "KDE reload failure reports successful rollback");
    expectTrue(reloadFailure.values == beforeReloadFailure,
               "KDE reload failure rolls back the config");
    expectTrue(reloadFailure.reloadCount == 2,
               "KDE reload failure retries notification after rollback");

    const qsizetype commandCount = reloadFailure.commands.size();
    error.clear();
    expectTrue(!reloadFailureManager.applyHttpProxy(
                   QStringLiteral("127.0.0.1"), 0, &error),
               "invalid KDE proxy port is rejected");
    expectTrue(error.contains(QStringLiteral("Invalid HTTP proxy port")),
               "invalid KDE proxy port has actionable error");
    expectTrue(reloadFailure.commands.size() == commandCount,
               "invalid KDE proxy port executes no command");

    zarya::SystemProxyState emptySnapshot;
    error.clear();
    expectTrue(!reloadFailureManager.restoreState(emptySnapshot, &error),
               "missing KDE snapshot is rejected");
    expectTrue(error.contains(QStringLiteral("Missing KDE kioslaverc snapshot")),
               "missing KDE snapshot has actionable error");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    testDesktopDetection();
    testLinuxBackendSelection();
    testGnomeSnapshotApplyRestore();
    testGnomeFailures();
    testKdeSnapshotApplyRestore();
    testKdeFallbackAndFailures();

    if (g_failures == 0) {
        std::fprintf(stdout, "All Linux proxy tests passed.\n");
    }
    return g_failures == 0 ? 0 : 1;
}
