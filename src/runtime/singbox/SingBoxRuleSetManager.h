#pragma once

#include <QStringList>

namespace zarya {

class SingBoxRuleSetManager {
public:
    static QString ruleSetDirectory();

    static QStringList expectedRuleSetFileNames(const QStringList& geoTags);

    static bool hasRuleSetFilesForTags(const QStringList& geoTags);

    static void appendGeoCompatibilityWarnings(const QStringList& geoTags,
                                               QStringList* warnings);
};

} // namespace zarya
