#pragma once

#include "platform/IAutostartManager.h"

#include <memory>

namespace zarya {

class AutostartManagerFactory {
public:
    static std::unique_ptr<IAutostartManager> create();
};

} // namespace zarya
