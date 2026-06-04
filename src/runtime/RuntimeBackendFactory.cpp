#include "runtime/RuntimeBackendFactory.h"

#include "runtime/singbox/SingBoxTunRuntimeBackend.h"
#include "runtime/xray/XraySystemProxyRuntimeBackend.h"
#include "storage/AppSettings.h"

namespace zarya {

RuntimeBackendFactory::RuntimeBackendFactory(CoreManager* coreManager)
    : m_coreManager(coreManager)
    , m_xrayBackend(new XraySystemProxyRuntimeBackend())
    , m_singBoxBackend(new SingBoxTunRuntimeBackend(coreManager))
{
}

RuntimeBackendFactory::~RuntimeBackendFactory()
{
    delete m_xrayBackend;
    delete m_singBoxBackend;
}

RuntimeMode RuntimeBackendFactory::effectiveRuntimeMode() const
{
    const AppSettings& settings = AppSettings::instance();
    if (!settings.enableExperimentalTun()) {
        return RuntimeMode::SystemProxyXray;
    }
    return settings.runtimeMode();
}

IRuntimeBackend* RuntimeBackendFactory::backendForMode(RuntimeMode mode)
{
    switch (mode) {
    case RuntimeMode::TunSingBoxExperimental:
        return m_singBoxBackend;
    case RuntimeMode::SystemProxyXray:
        return m_xrayBackend;
    }
    return m_xrayBackend;
}

IRuntimeBackend* RuntimeBackendFactory::activeBackend()
{
    return backendForMode(effectiveRuntimeMode());
}

SingBoxTunRuntimeBackend* RuntimeBackendFactory::singBoxTunBackend()
{
    return m_singBoxBackend;
}

XraySystemProxyRuntimeBackend* RuntimeBackendFactory::xraySystemProxyBackend()
{
    return m_xrayBackend;
}

} // namespace zarya
