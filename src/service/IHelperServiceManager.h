#pragma once

#include "service/HelperServiceInstallOptions.h"
#include "service/HelperServiceStatus.h"

#include <QObject>
#include <QString>

namespace zarya {

class IHelperServiceManager : public QObject {
    Q_OBJECT

public:
    explicit IHelperServiceManager(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~IHelperServiceManager() override = default;

    virtual QString backendName() const = 0;
    virtual bool isSupported() const = 0;

    virtual HelperServiceStatus status() = 0;

    virtual bool install(const HelperServiceInstallOptions& options, QString* errorMessage = nullptr) = 0;
    virtual bool uninstall(bool recoverKillSwitch, QString* errorMessage = nullptr) = 0;

    virtual bool start(QString* errorMessage = nullptr) = 0;
    virtual bool stop(QString* errorMessage = nullptr) = 0;
    virtual bool restart(QString* errorMessage = nullptr) = 0;

    virtual QString recoveryInstructions() const = 0;
    virtual QString manualInstallCommand(const HelperServiceInstallOptions& options) const = 0;

signals:
    void statusChanged();
};

} // namespace zarya
