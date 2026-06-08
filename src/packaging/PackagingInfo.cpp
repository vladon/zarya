#include "packaging/PackagingInfo.h"

#include <QCoreApplication>

namespace zarya {

QString PackagingInfo::versionString()
{
#ifndef ZARYA_VERSION_STRING
#define ZARYA_VERSION_STRING "0.21.0"
#endif
    return QStringLiteral(ZARYA_VERSION_STRING);
}

QString PackagingInfo::platformName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return QStringLiteral("unknown");
#endif
}

bool PackagingInfo::isBetaBuild()
{
    const QString version = versionString().toLower();
    if (version.contains(QStringLiteral("beta")) || version.contains(QStringLiteral("dev"))) {
        return true;
    }
    return version.startsWith(QStringLiteral("0."));
}

QString PackagingInfo::artifactPlatformTag()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows-x64");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux-x64");
#else
    return QStringLiteral("unknown");
#endif
}

} // namespace zarya
