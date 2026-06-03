#include "domain/ProfileSourceType.h"

namespace zarya {

QString profileSourceTypeToString(ProfileSourceType type)
{
    switch (type) {
    case ProfileSourceType::Manual:
        return QStringLiteral("manual");
    case ProfileSourceType::Subscription:
        return QStringLiteral("subscription");
    }
    return QStringLiteral("manual");
}

ProfileSourceType profileSourceTypeFromString(const QString& value,
                                              ProfileSourceType defaultValue)
{
    if (value.compare(QStringLiteral("subscription"), Qt::CaseInsensitive) == 0) {
        return ProfileSourceType::Subscription;
    }
    if (value.compare(QStringLiteral("manual"), Qt::CaseInsensitive) == 0) {
        return ProfileSourceType::Manual;
    }
    return defaultValue;
}

} // namespace zarya
