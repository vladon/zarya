#pragma once

#include "rulesets/RuleSetItem.h"

#include <QVector>

namespace zarya {

class RuleSetStore {
public:
    bool load(QVector<RuleSetItem>* customItems, QString* errorMessage = nullptr) const;
    bool save(const QVector<RuleSetItem>& customItems, QString* errorMessage = nullptr) const;
};

} // namespace zarya
