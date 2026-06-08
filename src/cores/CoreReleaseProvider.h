#pragma once

#include "cores/CoreRelease.h"
#include "domain/CoreType.h"

#include <QObject>

namespace zarya {

class CoreReleaseProvider : public QObject {
    Q_OBJECT

public:
    explicit CoreReleaseProvider(QObject* parent = nullptr) : QObject(parent) {}

    virtual CoreType coreType() const = 0;
    virtual QString providerName() const = 0;
    virtual void fetchLatestRelease(int timeoutMs) = 0;

signals:
    void latestReleaseReady(const CoreRelease& release);
    void error(const QString& message);
};

} // namespace zarya
