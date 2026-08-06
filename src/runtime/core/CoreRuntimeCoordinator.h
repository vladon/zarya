#pragma once

#include "domain/CoreType.h"

#include <QObject>
#include <optional>

namespace zarya {

class CoreRuntimeCoordinator : public QObject {
    Q_OBJECT

public:
    explicit CoreRuntimeCoordinator(QObject* parent = nullptr);

    bool acquire(CoreType type, QString* error = nullptr);
    void release(CoreType type);
    std::optional<CoreType> activeCore() const;

private:
    std::optional<CoreType> m_activeCore;
};

} // namespace zarya
