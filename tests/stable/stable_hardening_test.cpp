#include "diagnostics/DiagnosticsRedactor.h"
#include "features/FeatureGate.h"
#include "features/FeaturePolicy.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "storage/DefaultSettings.h"

#include <QCoreApplication>
#include <QJsonObject>

#include <cstdio>
#include <cstdlib>

namespace {

int g_failures = 0;

void expectTrue(bool condition, const char* message)
{
    if (!condition) {
        ++g_failures;
        fprintf(stderr, "FAIL: %s\n", message);
    }
}

void resetReleaseSettings()
{
    zarya::AppSettings& settings = zarya::AppSettings::instance();
    settings.setReleaseChannelKey(QStringLiteral("beta"));
    settings.setShowExperimentalFeatures(true);
    settings.setEnableExperimentalTun(false);
    settings.setRuntimeMode(zarya::RuntimeMode::SystemProxyXray);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    zarya::AppPaths::initialize(true);

    resetReleaseSettings();

    expectTrue(zarya::FeaturePolicy::defaultShowExperimentalFeatures(
                   zarya::ReleaseChannel::Stable) == false,
               "stable channel hides experimental by default");
    expectTrue(zarya::FeaturePolicy::defaultShowExperimentalFeatures(
                   zarya::ReleaseChannel::Rc) == false,
               "rc channel hides experimental by default");
    expectTrue(zarya::FeaturePolicy::defaultShowExperimentalFeatures(
                   zarya::ReleaseChannel::Beta),
               "beta channel shows experimental by default");
    expectTrue(!zarya::DefaultSettings::enablePortableUpdaterPoC(),
               "rc build disables portable updater install by default");

    zarya::AppSettings& settings = zarya::AppSettings::instance();
    settings.setReleaseChannelKey(QStringLiteral("rc"));
    settings.setShowExperimentalFeatures(false);
    settings.setEnableExperimentalTun(true);
    settings.setRuntimeMode(zarya::RuntimeMode::TunSingBoxExperimental);

    expectTrue(!zarya::FeatureGate::isVisible(zarya::FeatureId::SingBoxTunExperimental),
               "rc channel hides TUN feature");
    expectTrue(settings.effectiveRuntimeMode() == zarya::RuntimeMode::SystemProxyXray,
               "effective runtime falls back to Xray when TUN gated");
    expectTrue(settings.configuredRuntimeMode() == zarya::RuntimeMode::TunSingBoxExperimental,
               "configured runtime preserves user setting");

    settings.setReleaseChannelKey(QStringLiteral("beta"));
    settings.setShowExperimentalFeatures(true);
    expectTrue(zarya::FeatureGate::isVisible(zarya::FeatureId::SingBoxTunExperimental),
               "beta channel shows experimental features");

    const QString secret =
        QStringLiteral("helper.token=deadbeef vless://uuid@host:443?encryption=none#label");
    const QString redacted = zarya::DiagnosticsRedactor::redactText(
        secret, zarya::DiagnosticsRedactionMode::Strict);
    expectTrue(!redacted.contains(QStringLiteral("vless://")),
               "diagnostics redaction removes profile links");
    expectTrue(!redacted.contains(QStringLiteral("deadbeef")),
               "diagnostics redaction removes token-like secrets");

    const QJsonObject stableDiag = zarya::FeatureGate::diagnosticsJson();
    expectTrue(stableDiag.value(QStringLiteral("releaseChannel")).toString() == QStringLiteral("beta"),
               "diagnostics include release channel");
    expectTrue(stableDiag.contains(QStringLiteral("experimentalFeaturesEnabled")),
               "diagnostics include experimental feature state");

    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
