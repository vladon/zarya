#include "packaging/PublicBetaDocs.h"

#include "storage/AppPaths.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace zarya {

namespace {

QString resolveBundledDoc(const QString& fileName)
{
    AppPaths::initialize(AppPaths::isPortableMode());
    const QStringList candidates = {
        QDir(AppPaths::applicationDir()).filePath(QStringLiteral("docs/public-beta/") + fileName),
#if defined(Q_OS_MACOS)
        QDir(AppPaths::applicationDir())
            .filePath(QStringLiteral("../Resources/docs/public-beta/") + fileName),
#endif
    };
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return {};
}

} // namespace

QString PublicBetaDocs::bundledDocPath(const QString& fileName)
{
    return resolveBundledDoc(fileName);
}

bool PublicBetaDocs::openBundledDoc(const QString& fileName)
{
    const QString path = resolveBundledDoc(fileName);
    if (path.isEmpty()) {
        return false;
    }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QString PublicBetaDocs::projectIssuesUrl()
{
#ifdef ZARYA_PROJECT_ISSUES_URL
    const QString url = QStringLiteral(ZARYA_PROJECT_ISSUES_URL);
    if (!url.isEmpty()) {
        return url;
    }
#endif
    return {};
}

bool PublicBetaDocs::openIssueReporting()
{
    const QString issuesUrl = projectIssuesUrl();
    if (!issuesUrl.isEmpty()) {
        return QDesktopServices::openUrl(QUrl(issuesUrl));
    }
    return openBundledDoc(QStringLiteral("reporting-issues.md"));
}

} // namespace zarya
