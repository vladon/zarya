#pragma once

namespace zarya {

[[nodiscard]] constexpr bool shouldDisableUiAnimations(
    bool desktopEffectsEnabled,
    int widgetAnimationDuration) noexcept
{
    return !desktopEffectsEnabled || widgetAnimationDuration <= 0;
}

} // namespace zarya
