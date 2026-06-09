#include "app/BuildInfo.h"

#include "BuildInfo_config.h"

#include <QSysInfo>

namespace zarya {

QString BuildInfo::appVersion()
{
    return QStringLiteral(ZARYA_VERSION_STRING);
}

QString BuildInfo::buildCommit()
{
    return QStringLiteral(ZARYA_BUILD_COMMIT);
}

QString BuildInfo::buildDateUtc()
{
    return QStringLiteral(ZARYA_BUILD_DATE_UTC);
}

QString BuildInfo::buildChannel()
{
    return QStringLiteral(ZARYA_BUILD_CHANNEL);
}

QString BuildInfo::qtVersion()
{
    return QString::fromLatin1(QT_VERSION_STR);
}

QString BuildInfo::compilerInfo()
{
#if defined(_MSC_VER)
    return QStringLiteral("MSVC %1").arg(_MSC_VER);
#elif defined(__clang__)
    return QStringLiteral("Clang %1.%2.%3")
        .arg(__clang_major__)
        .arg(__clang_minor__)
        .arg(__clang_patchlevel__);
#elif defined(__GNUC__)
    return QStringLiteral("GCC %1.%2.%3")
        .arg(__GNUC__)
        .arg(__GNUC_MINOR__)
        .arg(__GNUC_PATCHLEVEL__);
#else
    return QStringLiteral("unknown");
#endif
}

QString BuildInfo::cliVersionText()
{
    return QStringLiteral("Zarya %1\nCommit: %2\nBuilt: %3\nChannel: %4\nQt: %5")
        .arg(appVersion(), buildCommit(), buildDateUtc(), buildChannel(), qtVersion());
}

QString BuildInfo::helperCliVersionText()
{
    return QStringLiteral("zarya-helper %1").arg(appVersion());
}

QString BuildInfo::aboutText()
{
    return QStringLiteral(
               "Zarya %1 (%2)\n"
               "Commit: %3\n"
               "Built: %4\n"
               "Qt %5\n\n"
               "Cross-platform proxy profile manager with Xray system proxy, "
               "subscriptions, routing, DNS, backup, and diagnostics.")
        .arg(appVersion(), buildChannel(), buildCommit(), buildDateUtc(), qtVersion());
}

} // namespace zarya
