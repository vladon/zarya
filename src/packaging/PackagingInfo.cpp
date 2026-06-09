#include "packaging/PackagingInfo.h"

#include "app/BuildInfo.h"

#include <QtGlobal>

namespace zarya {

QString PackagingInfo::versionString()
{
    return BuildInfo::appVersion();
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
    return BuildInfo::buildChannel().compare(QStringLiteral("beta"), Qt::CaseInsensitive) == 0
           || version.startsWith(QStringLiteral("0."));
}

QString PackagingInfo::artifactPlatformTag()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows-x64");
#elif defined(Q_OS_MACOS)
#if defined(__aarch64__) || defined(__arm64__)
    return QStringLiteral("macos-arm64");
#else
    return QStringLiteral("macos-x64");
#endif
#elif defined(Q_OS_LINUX)
#if defined(__aarch64__)
    return QStringLiteral("linux-arm64");
#else
    return QStringLiteral("linux-x64");
#endif
#else
    return QStringLiteral("unknown");
#endif
}

} // namespace zarya
