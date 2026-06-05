#include "rulesets/RequiredRuleSetDetector.h"

#include "dns/DnsGeoUtils.h"
#include "domain/DnsProfileMode.h"
#include "routing/RoutingGeoUtils.h"
#include "rulesets/RuleSetNormalizer.h"
#include "storage/AppPaths.h"

#include <QFile>

namespace zarya {

namespace {

void appendRequired(QVector<RequiredRuleSet>* output, const QString& reference,
                    const QString& sourceArea)
{
    const QString tag = RuleSetNormalizer::normalizeGeoReference(reference);
    if (tag.isEmpty()) {
        return;
    }
    for (const RequiredRuleSet& existing : *output) {
        if (existing.tag == tag) {
            return;
        }
    }

    RequiredRuleSet required;
    required.normalizedId = tag;
    required.tag = tag;
    required.originalValue = reference.trimmed();
    required.sourceArea = sourceArea;
    const QString srsPath = AppPaths::localRuleSetSrsPath(tag);
    required.localPath = srsPath;
    required.available = QFile::exists(srsPath);
    if (!required.available) {
        required.warning =
            QStringLiteral("Rule set %1 is missing (%2).").arg(tag, reference.trimmed());
    }
    output->append(required);
}

} // namespace

QVector<RequiredRuleSet> RequiredRuleSetDetector::detect(const RoutingProfile& routingProfile,
                                                           const DnsProfile& dnsProfile)
{
    QVector<RequiredRuleSet> required;

    for (const QString& value : RoutingGeoUtils::geoTagsUsed(routingProfile)) {
        appendRequired(&required, value, QStringLiteral("routing"));
    }

    if (dnsProfile.mode == DnsProfileMode::ChinaDirectGlobalRemote) {
        appendRequired(&required, QStringLiteral("geosite:cn"), QStringLiteral("dns"));
        appendRequired(&required, QStringLiteral("geosite:geolocation-!cn"), QStringLiteral("dns"));
    }

    for (const QString& value : DnsGeoUtils::geoReferencesUsed(dnsProfile)) {
        appendRequired(&required, value, QStringLiteral("dns"));
    }

    return required;
}

} // namespace zarya
