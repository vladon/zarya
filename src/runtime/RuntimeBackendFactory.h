#pragma once

#include "runtime/IRuntimeBackend.h"
#include "runtime/RuntimeBackendType.h"
#include "runtime/singbox/SingBoxTunRuntimeBackend.h"
#include "runtime/xray/XraySystemProxyRuntimeBackend.h"

namespace zarya {

class CoreManager;
class CoreRuntimeCoordinator;
class EmbeddedXrayRuntimeHost;

class RuntimeBackendFactory {
public:
    explicit RuntimeBackendFactory(CoreManager* coreManager);
    ~RuntimeBackendFactory();

    RuntimeMode effectiveRuntimeMode() const;
    IRuntimeBackend* backendForMode(RuntimeMode mode);
    IRuntimeBackend* activeBackend();

    SingBoxTunRuntimeBackend* singBoxTunBackend();
    XraySystemProxyRuntimeBackend* xraySystemProxyBackend();
    EmbeddedXrayRuntimeHost* embeddedXrayHost();

private:
    CoreManager* m_coreManager = nullptr;
    CoreRuntimeCoordinator* m_runtimeCoordinator = nullptr;
    EmbeddedXrayRuntimeHost* m_embeddedXrayHost = nullptr;
    XraySystemProxyRuntimeBackend* m_xrayBackend = nullptr;
    SingBoxTunRuntimeBackend* m_singBoxBackend = nullptr;
};

} // namespace zarya
