#pragma once

#include <QHash>
#include <QString>

namespace zarya {

struct SingBoxRuleSetContext {
    QHash<QString, QString> tagToLocalPath;
    bool requireLocalRuleSets = false;
    bool useRuleSetReferences = true;

    bool hasLocalRuleSet(const QString& tag) const;
    QString localPathForTag(const QString& tag) const;
};

} // namespace zarya
