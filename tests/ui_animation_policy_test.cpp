#include "ui/desktopapp/UiAnimationPolicy.h"

#include <iostream>

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

} // namespace

int main()
{
    bool ok = true;
    ok &= expect(
        !zarya::shouldDisableUiAnimations(true, 120),
        "animations should remain enabled when desktop effects and duration are enabled");
    ok &= expect(
        zarya::shouldDisableUiAnimations(false, 120),
        "disabled desktop effects should disable animations");
    ok &= expect(
        zarya::shouldDisableUiAnimations(true, 0),
        "a zero animation duration should disable animations");
    ok &= expect(
        zarya::shouldDisableUiAnimations(true, -1),
        "an unavailable animation duration should disable animations safely");
    return ok ? 0 : 1;
}
