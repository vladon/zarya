#pragma once

#include <QString>

namespace zarya {

enum class AppVersionPrerelease {
    None,
    Beta,
    Dev,
    Other,
};

struct AppVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    AppVersionPrerelease prerelease = AppVersionPrerelease::None;

    static AppVersion parse(const QString& text);
    static int compare(const QString& left, const QString& right);
    static bool isLessThan(const QString& left, const QString& right);
    static bool isGreaterThan(const QString& left, const QString& right);
};

} // namespace zarya
