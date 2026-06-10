#pragma once

#include "platform/SystemProxyState.h"

namespace zarya {

class SystemProxyStateStore {
public:
    static bool save(const SystemProxyState& state);
    static bool load(SystemProxyState* state);
    static void clear();
    static bool exists();
};

} // namespace zarya
