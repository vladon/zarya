#include "runtime/core/CoreRuntimeCoordinator.h"

#include "domain/CoreType.h"

namespace zarya {

CoreRuntimeCoordinator::CoreRuntimeCoordinator(QObject* parent)
    : QObject(parent)
{
}

bool CoreRuntimeCoordinator::acquire(CoreType type, QString* error)
{
    if (m_activeCore.has_value() && m_activeCore.value() != type) {
        if (error) {
            *error = QStringLiteral("%1 is already active.")
                         .arg(coreTypeToString(m_activeCore.value()));
        }
        return false;
    }
    if (m_activeCore.has_value()) {
        if (error) {
            *error = QStringLiteral("%1 already has an active runtime instance.")
                         .arg(coreTypeToString(type));
        }
        return false;
    }
    m_activeCore = type;
    if (error) {
        error->clear();
    }
    return true;
}

void CoreRuntimeCoordinator::release(CoreType type)
{
    if (m_activeCore == type) {
        m_activeCore.reset();
    }
}

std::optional<CoreType> CoreRuntimeCoordinator::activeCore() const
{
    return m_activeCore;
}

} // namespace zarya
