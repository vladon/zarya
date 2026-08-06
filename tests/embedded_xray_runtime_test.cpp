#include "runtime/core/CoreRuntimeCoordinator.h"
#include "runtime/embedded/xray/EmbeddedXrayRuntimeHost.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <iostream>

namespace {

int fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    zarya::CoreRuntimeCoordinator coordinator;
    zarya::EmbeddedXrayRuntimeHost host(&coordinator);

#if defined(ZARYA_EMBEDDED_XRAY_DISABLED)
    if (host.isAvailable()) {
        return fail("disabled embedded host unexpectedly available");
    }
    if (host.loadStatus().isEmpty()) {
        return fail("disabled embedded host has no load status");
    }
#else
    if (!host.isAvailable()) {
        std::cerr << host.loadStatus().toStdString() << '\n';
        return fail("embedded host unavailable");
    }
    if (host.abiVersion() != 1 || host.version().isEmpty()) {
        return fail("embedded host version contract failed");
    }
#if !defined(ZARYA_EMBEDDED_XRAY_STATIC_ABI)
    const QFileInfo library(host.libraryPath());
    if (!library.isAbsolute() || !library.isFile()) {
        return fail("embedded host did not use an absolute existing library path");
    }
    if (QDir::cleanPath(library.absolutePath())
        != QDir::cleanPath(QCoreApplication::applicationDirPath())) {
        return fail("embedded host loaded outside the application directory");
    }
#endif
#endif

    const zarya::CoreOperationResult firstStop = host.stop();
    const zarya::CoreOperationResult secondStop = host.stop();
    if (!firstStop.success || !secondStop.success) {
        return fail("embedded stop is not idempotent");
    }

#if !defined(ZARYA_EMBEDDED_XRAY_DISABLED)
    zarya::CoreLaunchRequest invalid;
    invalid.coreType = zarya::CoreType::Xray;
    invalid.configJson = QByteArrayLiteral("{invalid-json vless://secret.example}");
    invalid.assetDir = QCoreApplication::applicationDirPath();
    const zarya::CoreOperationResult validation = host.validate(invalid);
    if (validation.success) {
        return fail("invalid config unexpectedly passed validation");
    }
    if (validation.message.contains(QStringLiteral("vless://"))) {
        return fail("validation error leaked a share link");
    }
#endif

    return 0;
}
