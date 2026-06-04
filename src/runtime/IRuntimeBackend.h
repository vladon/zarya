#pragma once

#include "domain/Profile.h"
#include "runtime/RuntimeBackendType.h"
#include "runtime/RuntimeStartOptions.h"

#include <QObject>
#include <QString>

namespace zarya {

class IRuntimeBackend : public QObject {
    Q_OBJECT

public:
    explicit IRuntimeBackend(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~IRuntimeBackend() = default;

    virtual QString displayName() const = 0;
    virtual RuntimeBackendType type() const = 0;

    virtual bool isSupported(QString* reason = nullptr) const = 0;
    virtual bool validateProfile(const Profile& profile, QString* reason = nullptr) const = 0;

    virtual bool start(const Profile& profile, const RuntimeStartOptions& options) = 0;
    virtual bool stop() = 0;
    virtual bool isRunning() const = 0;

signals:
    void logLine(const QString& line);
    void stateChanged(RuntimeState state);
    void errorOccurred(const QString& message);
};

} // namespace zarya
