#pragma once

#include <QString>

namespace zarya {

enum class ThemeMode {
    System,
    Light,
    Dark,
};

inline QString themeModeToString(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Light:
        return QStringLiteral("light");
    case ThemeMode::Dark:
        return QStringLiteral("dark");
    case ThemeMode::System:
    default:
        return QStringLiteral("system");
    }
}

inline ThemeMode themeModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("light")) {
        return ThemeMode::Light;
    }
    if (normalized == QStringLiteral("dark")) {
        return ThemeMode::Dark;
    }
    return ThemeMode::System;
}

} // namespace zarya
