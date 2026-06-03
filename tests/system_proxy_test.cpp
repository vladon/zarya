#include "platform/SystemProxyDebug.h"
#include "platform/SystemProxyManagerFactory.h"

#include <QCoreApplication>
#include <QString>

#include <cstdio>
#include <memory>

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

bool stateEquals(const zarya::SystemProxyState& a, const zarya::SystemProxyState& b)
{
    return a.proxyEnabled == b.proxyEnabled && a.proxyServer == b.proxyServer
           && a.proxyOverride == b.proxyOverride && a.autoDetect == b.autoDetect
           && a.autoConfigUrl == b.autoConfigUrl;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const std::unique_ptr<zarya::ISystemProxyManager> manager =
        zarya::SystemProxyManagerFactory::create();

    if (!manager->isSupported()) {
        return pass("System proxy not supported on this platform (stub)");
    }

    bool ok = true;

    QString error;
    const zarya::SystemProxyState before = manager->readCurrentState(&error);
    if (!error.isEmpty()) {
        return fail(error.toUtf8().constData()) ? 1 : 1;
    }
    ok &= pass("Read current proxy state");

    if (!manager->applyHttpProxy(QStringLiteral("127.0.0.1"), 10809, &error)) {
        ok &= fail(error.toUtf8().constData());
    } else {
        ok &= pass("Apply local HTTP proxy");
    }

    const zarya::SystemProxyState during = manager->readCurrentState(&error);
    if (!during.proxyEnabled) {
        ok &= fail("ProxyEnable not set after apply");
    }
    if (!during.proxyServer.contains(QStringLiteral("127.0.0.1:10809"))) {
        ok &= fail("ProxyServer does not contain expected endpoint");
    } else {
        ok &= pass("ProxyServer set to local HTTP inbound");
    }
    if (during.proxyOverride != QStringLiteral("<local>")) {
        ok &= fail("ProxyOverride is not <local>");
    } else {
        ok &= pass("ProxyOverride is <local>");
    }

    if (!manager->restoreState(before, &error)) {
        ok &= fail(error.toUtf8().constData());
    } else {
        ok &= pass("Restore previous proxy state");
    }

    const zarya::SystemProxyState after = manager->readCurrentState(&error);
    if (!stateEquals(after, before)) {
        std::fprintf(stderr, "FAIL: restored state mismatch\nBefore:\n%s\nAfter:\n%s\n",
                     zarya::formatSystemProxyStateForLog(before).toUtf8().constData(),
                     zarya::formatSystemProxyStateForLog(after).toUtf8().constData());
        ok = false;
    } else {
        ok &= pass("Restored state matches saved state");
    }

    return ok ? 0 : 1;
}
