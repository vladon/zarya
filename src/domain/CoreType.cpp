#include "domain/CoreType.h"

namespace zarya {

QString coreTypeToString(CoreType type)
{
    switch (type) {
    case CoreType::Xray:
        return QStringLiteral("Xray");
    case CoreType::SingBox:
        return QStringLiteral("SingBox");
    }
    return QStringLiteral("Xray");
}

CoreType coreTypeFromString(const QString& value, CoreType defaultValue)
{
    if (value.compare(QStringLiteral("Xray"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("xray"), Qt::CaseInsensitive) == 0) {
        return CoreType::Xray;
    }
    if (value.compare(QStringLiteral("SingBox"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("sing-box"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("singbox"), Qt::CaseInsensitive) == 0) {
        return CoreType::SingBox;
    }
    return defaultValue;
}

} // namespace zarya
