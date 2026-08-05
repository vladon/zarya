#include "ui/theme/ThemeTokens.h"

#include <QColor>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

constexpr double kMinimumTextContrast = 4.5;

struct NamedColor {
    const char* name;
    QColor color;
};

struct SemanticPair {
    const char* name;
    QColor foreground;
    QColor surface;
};

double linearizedChannel(int channel)
{
    const double value = static_cast<double>(channel) / 255.0;
    return value <= 0.04045
        ? value / 12.92
        : std::pow((value + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor& color)
{
    return 0.2126 * linearizedChannel(color.red())
        + 0.7152 * linearizedChannel(color.green())
        + 0.0722 * linearizedChannel(color.blue());
}

double contrastRatio(const QColor& first, const QColor& second)
{
    const double firstLuminance = relativeLuminance(first);
    const double secondLuminance = relativeLuminance(second);
    const double lighter = std::max(firstLuminance, secondLuminance);
    const double darker = std::min(firstLuminance, secondLuminance);
    return (lighter + 0.05) / (darker + 0.05);
}

bool expectContrast(
    const char* theme,
    const std::string& pair,
    const QColor& foreground,
    const QColor& background)
{
    const double ratio = contrastRatio(foreground, background);
    if (foreground.isValid() && background.isValid()
        && foreground.alpha() == 255 && background.alpha() == 255
        && ratio >= kMinimumTextContrast) {
        return true;
    }

    std::cerr << theme << " theme: " << pair << " contrast is "
              << std::fixed << std::setprecision(2) << ratio
              << ":1, expected at least " << kMinimumTextContrast << ":1\n";
    return false;
}

bool verifyTheme(const char* name, const zarya::ThemeTokens& tokens)
{
    bool ok = true;
    const std::array neutralSurfaces = {
        NamedColor{"window", tokens.windowBg},
        NamedColor{"surface", tokens.surfaceBg},
        NamedColor{"panel", tokens.panelBg},
    };
    const std::array neutralText = {
        NamedColor{"primary text", tokens.textPrimary},
        NamedColor{"secondary text", tokens.textSecondary},
        NamedColor{"accent", tokens.accent},
        NamedColor{"accent hover", tokens.accentHover},
    };
    for (const NamedColor& foreground : neutralText) {
        for (const NamedColor& background : neutralSurfaces) {
            ok &= expectContrast(
                name,
                std::string(foreground.name) + " / " + background.name,
                foreground.color,
                background.color);
        }
    }

    ok &= expectContrast(
        name, "accent foreground / fill", tokens.accentFg, tokens.accentFill);
    ok &= expectContrast(
        name,
        "accent foreground / hover fill",
        tokens.accentFg,
        tokens.accentFillHover);

    const std::array semanticPairs = {
        SemanticPair{"success", tokens.success, tokens.successSurface},
        SemanticPair{"warning", tokens.warning, tokens.warningSurface},
        SemanticPair{"danger", tokens.danger, tokens.dangerSurface},
        SemanticPair{"info", tokens.info, tokens.infoSurface},
        SemanticPair{"experimental", tokens.experimental, tokens.experimentalSurface},
    };
    for (const SemanticPair& semantic : semanticPairs) {
        ok &= expectContrast(
            name,
            std::string(semantic.name) + " / semantic surface",
            semantic.foreground,
            semantic.surface);
        ok &= expectContrast(
            name,
            std::string(semantic.name) + " / window",
            semantic.foreground,
            tokens.windowBg);
        ok &= expectContrast(
            name,
            std::string(semantic.name) + " / surface",
            semantic.foreground,
            tokens.surfaceBg);
        ok &= expectContrast(
            name,
            std::string("primary text / ") + semantic.name + " surface",
            tokens.textPrimary,
            semantic.surface);
    }
    return ok;
}

} // namespace

int main()
{
    const bool lightOk = verifyTheme("light", zarya::lightTokens());
    const bool darkOk = verifyTheme("dark", zarya::darkTokens());
    return lightOk && darkOk ? 0 : 1;
}
