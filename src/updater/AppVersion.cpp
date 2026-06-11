#include "updater/AppVersion.h"

#include <QStringList>

namespace zarya {

namespace {

int prereleaseRank(AppVersionPrerelease prerelease)
{
    switch (prerelease) {
    case AppVersionPrerelease::None:
        return 3;
    case AppVersionPrerelease::Beta:
        return 2;
    case AppVersionPrerelease::Dev:
        return 1;
    case AppVersionPrerelease::Other:
        return 0;
    }
    return 0;
}

AppVersionPrerelease parsePrerelease(const QString& suffix)
{
    const QString lower = suffix.trimmed().toLower();
    if (lower.isEmpty()) {
        return AppVersionPrerelease::None;
    }
    if (lower == QStringLiteral("beta")) {
        return AppVersionPrerelease::Beta;
    }
    if (lower == QStringLiteral("dev") || lower == QStringLiteral("alpha")) {
        return AppVersionPrerelease::Dev;
    }
    return AppVersionPrerelease::Other;
}

} // namespace

AppVersion AppVersion::parse(const QString& text)
{
    AppVersion version;
    QString normalized = text.trimmed();
    if (normalized.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        normalized = normalized.mid(1);
    }

    QString prereleaseSuffix;
    const int dashIndex = normalized.indexOf(QLatin1Char('-'));
    if (dashIndex >= 0) {
        prereleaseSuffix = normalized.mid(dashIndex + 1);
        normalized = normalized.left(dashIndex);
    }

    const QStringList parts = normalized.split(QLatin1Char('.'));
    if (parts.size() > 0) {
        version.major = parts.at(0).toInt();
    }
    if (parts.size() > 1) {
        version.minor = parts.at(1).toInt();
    }
    if (parts.size() > 2) {
        version.patch = parts.at(2).toInt();
    }
    version.prerelease = parsePrerelease(prereleaseSuffix);
    return version;
}

int AppVersion::compare(const QString& left, const QString& right)
{
    const AppVersion leftVersion = parse(left);
    const AppVersion rightVersion = parse(right);

    if (leftVersion.major != rightVersion.major) {
        return leftVersion.major < rightVersion.major ? -1 : 1;
    }
    if (leftVersion.minor != rightVersion.minor) {
        return leftVersion.minor < rightVersion.minor ? -1 : 1;
    }
    if (leftVersion.patch != rightVersion.patch) {
        return leftVersion.patch < rightVersion.patch ? -1 : 1;
    }

    const int leftRank = prereleaseRank(leftVersion.prerelease);
    const int rightRank = prereleaseRank(rightVersion.prerelease);
    if (leftRank != rightRank) {
        return leftRank < rightRank ? -1 : 1;
    }
    return 0;
}

bool AppVersion::isLessThan(const QString& left, const QString& right)
{
    return compare(left, right) < 0;
}

bool AppVersion::isGreaterThan(const QString& left, const QString& right)
{
    return compare(left, right) > 0;
}

} // namespace zarya
