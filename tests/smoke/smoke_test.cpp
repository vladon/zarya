#include "diagnostics/DiagnosticsRedactor.h"
#include "storage/DefaultSettings.h"
#include "storage/SettingsValidator.h"
#include "subscription/ShareLinkParser.h"

#include <QCoreApplication>
#include <cstdio>

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

bool testSettingsValidation()
{
    const zarya::SettingsValidationResult result = zarya::SettingsValidator::validateAndFixOnStartup();
    if (!result.ok && !result.errors.isEmpty()) {
        return fail("settings validation returned errors");
    }
    return pass("settings validation");
}

bool testShareLinkParser()
{
    const QString vless = QStringLiteral(
        "vless://11111111-1111-1111-1111-111111111111@example.com:443?type=tcp&security=reality#Test");
    zarya::ShareLinkParseResult result = zarya::ShareLinkParser::parse(vless);
    if (!result.ok) {
        return fail("vless share link parse");
    }
    return pass("share link parser vless");
}

bool testDiagnosticsRedaction()
{
    const QString secret =
        QStringLiteral("vless://uuid@host:443?encryption=none#label password=abc123");
    const QString redacted = zarya::DiagnosticsRedactor::redactText(
        secret, zarya::DiagnosticsRedactionMode::Strict);
    if (redacted.contains(QStringLiteral("vless://"))) {
        return fail("diagnostics redaction left vless URI");
    }
    return pass("diagnostics redaction");
}

bool testDefaultSettings()
{
    if (zarya::DefaultSettings::enableExperimentalTun()) {
        return fail("TUN should be disabled by default");
    }
    if (zarya::DefaultSettings::enableExperimentalKillSwitch()) {
        return fail("kill switch should be disabled by default");
    }
    if (zarya::DefaultSettings::allowCoreUpdateWithoutChecksum()) {
        return fail("checksum bypass should be disabled by default");
    }
    return pass("beta default settings");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    int failures = 0;
    if (!testSettingsValidation()) {
        ++failures;
    }
    if (!testShareLinkParser()) {
        ++failures;
    }
    if (!testDiagnosticsRedaction()) {
        ++failures;
    }
    if (!testDefaultSettings()) {
        ++failures;
    }

    if (failures == 0) {
        std::fprintf(stdout, "All smoke tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d smoke test(s) failed.\n", failures);
    return 1;
}
