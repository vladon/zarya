#include "runtime/singbox/SingBoxRuleSetManager.h"

#include "storage/AppPaths.h"

#include <QDir>
#include <QFileInfo>

namespace zarya {

QString SingBoxRuleSetManager::ruleSetDirectory()
{
    return AppPaths::singBoxRuleSetDir();
}

QStringList SingBoxRuleSetManager::expectedRuleSetFileNames(const QStringList& geoTags)
{
    QStringList names;
    for (const QString& tag : geoTags) {
        const QString normalized = tag.trimmed().toLower();
        if (normalized.isEmpty()) {
            continue;
        }
        names.append(QStringLiteral("geosite-%1.srs").arg(normalized));
        names.append(QStringLiteral("geoip-%1.srs").arg(normalized));
    }
    return names;
}

bool SingBoxRuleSetManager::hasRuleSetFilesForTags(const QStringList& geoTags)
{
    const QStringList expected = expectedRuleSetFileNames(geoTags);
    if (expected.isEmpty()) {
        return true;
    }
    const QDir dir(ruleSetDirectory());
    for (const QString& name : expected) {
        if (dir.exists(name)) {
            return true;
        }
    }
    return false;
}

void SingBoxRuleSetManager::appendGeoCompatibilityWarnings(const QStringList& geoTags,
                                                           QStringList* warnings)
{
    if (!warnings || geoTags.isEmpty()) {
        return;
    }
    warnings->append(
        QStringLiteral("geosite/geoip rules may require sing-box rule-set files depending on your "
                       "sing-box version. Xray geoip.dat/geosite.dat are not the same artifact. "
                       "Rule-set update manager is not implemented yet."));
    if (!hasRuleSetFilesForTags(geoTags)) {
        const QStringList expected = expectedRuleSetFileNames(geoTags);
        warnings->append(
            QStringLiteral("No sing-box rule-set files found under %1 (expected examples: %2).")
                .arg(ruleSetDirectory(), expected.join(QStringLiteral(", "))));
    }
}

} // namespace zarya
