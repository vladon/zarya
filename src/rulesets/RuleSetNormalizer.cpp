#include "rulesets/RuleSetNormalizer.h"

namespace zarya {

QString RuleSetNormalizer::normalizeGeoReference(const QString& reference)
{
    const QString trimmed = reference.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (isGeoSiteReference(trimmed)) {
        return geositeTagFromValue(trimmed.mid(8));
    }
    if (isGeoIpReference(trimmed)) {
        const QString value = trimmed.mid(6).trimmed().toLower();
        if (value == QStringLiteral("private")) {
            return {};
        }
        return geoipTagFromValue(value);
    }
    return {};
}

QString RuleSetNormalizer::geositeTagFromValue(const QString& geoValue)
{
    QString normalized = geoValue.trimmed().toLower();
    if (normalized == QStringLiteral("geolocation-!cn")) {
        normalized = QStringLiteral("geolocation-not-cn");
    }
    normalized.replace(QLatin1Char('/'), QLatin1Char('-'));
    return QStringLiteral("geosite-%1").arg(normalized);
}

QString RuleSetNormalizer::geoipTagFromValue(const QString& geoValue)
{
    QString normalized = geoValue.trimmed().toLower();
    normalized.replace(QLatin1Char('/'), QLatin1Char('-'));
    return QStringLiteral("geoip-%1").arg(normalized);
}

bool RuleSetNormalizer::isGeoSiteReference(const QString& reference)
{
    return reference.trimmed().startsWith(QStringLiteral("geosite:"), Qt::CaseInsensitive);
}

bool RuleSetNormalizer::isGeoIpReference(const QString& reference)
{
    return reference.trimmed().startsWith(QStringLiteral("geoip:"), Qt::CaseInsensitive);
}

} // namespace zarya
