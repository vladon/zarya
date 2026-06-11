#pragma once

#include "service/IHelperServiceManager.h"

namespace zarya {

class StubHelperServiceManager : public IHelperServiceManager {
    Q_OBJECT

public:
    explicit StubHelperServiceManager(QObject* parent = nullptr);

    QString backendName() const override;
    bool isSupported() const override;
    HelperServiceStatus status() override;
    bool install(const HelperServiceInstallOptions& options, QString* errorMessage = nullptr) override;
    bool uninstall(bool recoverKillSwitch, QString* errorMessage = nullptr) override;
    bool start(QString* errorMessage = nullptr) override;
    bool stop(QString* errorMessage = nullptr) override;
    bool restart(QString* errorMessage = nullptr) override;
    QString recoveryInstructions() const override;
    QString manualInstallCommand(const HelperServiceInstallOptions& options) const override;
};

} // namespace zarya
