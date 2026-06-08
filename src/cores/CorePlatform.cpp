#include "cores/CorePlatform.h"

#include <QSysInfo>

namespace zarya {

namespace {

QString normalizeArch(const QString& arch)
{
    const QString lower = arch.toLower();
    if (lower.contains(QStringLiteral("arm64")) || lower.contains(QStringLiteral("aarch64"))) {
        return QStringLiteral("arm64");
    }
    if (lower.contains(QStringLiteral("x86_64")) || lower.contains(QStringLiteral("amd64"))
        || lower.contains(QStringLiteral("i386")) || lower.contains(QStringLiteral("x86"))) {
        return QStringLiteral("amd64");
    }
    return lower;
}

QString osToken()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("darwin");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return QStringLiteral("unknown");
#endif
}

} // namespace

CorePlatformInfo currentCorePlatform()
{
    CorePlatformInfo info;
    info.osToken = osToken();
    info.archToken = normalizeArch(QSysInfo::currentCpuArchitecture());
    info.displayName = QStringLiteral("%1-%2").arg(info.osToken, info.archToken);
    return info;
}

} // namespace zarya
