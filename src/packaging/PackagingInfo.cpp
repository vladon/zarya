#include "packaging/PackagingInfo.h"

#include <QCoreApplication>

namespace zarya {

QString PackagingInfo::versionString()
{
#ifndef ZARYA_VERSION_STRING
#define ZARYA_VERSION_STRING "0.14.0"
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
