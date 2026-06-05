#pragma once

#include "rulesets/RuleSetItem.h"

#include <QVector>

namespace zarya {

class RuleSetCatalog {
public:
    static QVector<RuleSetItem> builtInItems();
    static RuleSetItem* findByTag(QVector<RuleSetItem>& items, const QString& tag);
    static const RuleSetItem* findByTag(const QVector<RuleSetItem>& items, const QString& tag);
};

} // namespace zarya
