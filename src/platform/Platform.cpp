#include "platform/Platform.h"

#include "storage/AppPaths.h"

#include <QDir>
#include <QFileInfo>

namespace zarya {

namespace {

QString coresBaseDir()
{
    return AppPaths::coresDir();
}

QString withExecutableName(const QString& relativePath, const QString& windowsName,
                           const QString& unixName)
{
#ifdef Q_OS_WIN
    return QDir(coresBaseDir()).filePath(relativePath + QLatin1Char('/') + windowsName);
#else
    return QDir(coresBaseDir()).filePath(relativePath + QLatin1Char('/') + unixName);
#endif
}

} // namespace

QString Platform::defaultXrayExecutablePath()
{
    return withExecutableName(QStringLiteral("xray"),
                              QStringLiteral("xray.exe"),
                              QStringLiteral("xray"));
}

QString Platform::defaultSingBoxExecutablePath()
{
    return withExecutableName(QStringLiteral("sing-box"),
                              QStringLiteral("sing-box.exe"),
                              QStringLiteral("sing-box"));
}

} // namespace zarya
