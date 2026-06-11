#include "service/HelperServiceIdentity.h"

namespace zarya {

QString HelperServiceIdentity::internalServiceName()
{
    return QStringLiteral("ZaryaHelper");
}

QString HelperServiceIdentity::displayName()
{
    return QStringLiteral("Zarya Helper");
}

QString HelperServiceIdentity::macOsLabel()
{
    return QStringLiteral("dev.vladon.zarya.helper");
}

QString HelperServiceIdentity::linuxUnitName()
{
    return QStringLiteral("zarya-helper.service");
}

QString HelperServiceIdentity::polkitActionPrefix()
{
    return QStringLiteral("dev.vladon.zarya.helper");
}

} // namespace zarya
