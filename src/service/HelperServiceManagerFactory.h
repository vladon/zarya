#pragma once

#include <memory>

namespace zarya {

class IHelperServiceManager;

class HelperServiceManagerFactory {
public:
    static std::unique_ptr<IHelperServiceManager> create();
};

} // namespace zarya
