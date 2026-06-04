#pragma once

#include "runtime/IRuntimeBackend.h"
#include "runtime/RuntimeBackendType.h"
#include "runtime/singbox/SingBoxTunRuntimeBackend.h"
#include "runtime/xray/XraySystemProxyRuntimeBackend.h"

namespace zarya {

class CoreManager;

class RuntimeBackendFactory {
public:
    explicit RuntimeBackendFactory(CoreManager* coreManager);
    ~RuntimeBackendFactory();

    RuntimeMode effectiveRuntimeMode() const;
    IRuntimeBackend* backendForMode(RuntimeMode mode);
    IRuntimeBackend* activeBackend();

    SingBoxTunRuntimeBackend* singBoxTunBackend();
    XraySystemProxyRuntimeBackend* xraySystemProxyBackend();

private:
    CoreManager* m_coreManager = nullptr;
    XraySystemProxyRuntimeBackend* m_xrayBackend = nullptr;
    SingBoxTunRuntimeBackend* m_singBoxBackend = nullptr;
};

} // namespace zarya
