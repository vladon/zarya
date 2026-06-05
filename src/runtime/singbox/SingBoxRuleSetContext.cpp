#include "runtime/singbox/SingBoxRuleSetContext.h"

namespace zarya {

bool SingBoxRuleSetContext::hasLocalRuleSet(const QString& tag) const
{
    return tagToLocalPath.contains(tag) && !tagToLocalPath.value(tag).isEmpty();
}

QString SingBoxRuleSetContext::localPathForTag(const QString& tag) const
{
    return tagToLocalPath.value(tag);
}

} // namespace zarya
