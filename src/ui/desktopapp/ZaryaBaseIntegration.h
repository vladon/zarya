#pragma once

#include "base/integration.h"

namespace zarya {

class ZaryaBaseIntegration final : public base::Integration {
public:
    ZaryaBaseIntegration(int argc, char** argv);

    void enterFromEventLoop(FnMut<void()>&& method) override;
    bool logSkipDebug() override;
    void logMessageDebug(const QString& message) override;
    void logMessage(const QString& message) override;
};

} // namespace zarya
