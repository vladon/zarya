#pragma once

#include "platform/ISystemProxyManager.h"

#include <memory>

namespace zarya {

class SystemProxyManagerFactory {
public:
    static std::unique_ptr<ISystemProxyManager> create();
};

} // namespace zarya
