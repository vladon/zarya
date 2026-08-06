#pragma once

#include "runtime/core/CoreRuntimeTypes.h"

#include <QObject>

namespace zarya {

class ICoreRuntimeHost : public QObject {
    Q_OBJECT

public:
    explicit ICoreRuntimeHost(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~ICoreRuntimeHost() override = default;

    virtual CoreDistributionKind distributionKind() const = 0;
    virtual CoreRuntimeCapabilities capabilities() const = 0;
    virtual bool isAvailable() const = 0;
    virtual QString version() const = 0;
    virtual int abiVersion() const = 0;
    virtual QString loadStatus() const = 0;
    virtual CoreRuntimeState state() const = 0;

    virtual CoreOperationResult validate(const CoreLaunchRequest& request) = 0;
    virtual CoreOperationResult start(const CoreLaunchRequest& request) = 0;
    virtual CoreOperationResult stop() = 0;

signals:
    void stateChanged(zarya::CoreRuntimeState state);
    void logLine(const QString& line);
    void errorOccurred(const QString& errorCode, const QString& message);
};

} // namespace zarya
