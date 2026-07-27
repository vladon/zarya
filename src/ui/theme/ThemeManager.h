#pragma once

#include "ui/theme/ThemeMode.h"
#include "ui/theme/ThemeTokens.h"

#include <QObject>

namespace zarya {

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();

    ThemeMode mode() const;
    void setMode(ThemeMode mode);

    bool effectiveIsDark() const;
    ThemeTokens tokens() const;

    void apply();

Q_SIGNALS:
    void themeChanged();

private:
    explicit ThemeManager(QObject* parent = nullptr);

    bool resolveIsDark(ThemeMode mode) const;
    QString buildStyleSheet(const ThemeTokens& tokens) const;
    void applyPalette(const ThemeTokens& tokens) const;

    bool m_fusionApplied = false;
    bool m_hintsConnected = false;
    ThemeTokens m_tokens;
    bool m_isDark = false;
};

} // namespace zarya
