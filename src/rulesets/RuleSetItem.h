#pragma once

#include "rulesets/RuleSetKind.h"
#include "rulesets/RuleSetStatus.h"

#include <QDateTime>
#include <QUrl>
#include <QString>

namespace zarya {

struct RuleSetItem {
    QString id;
    QString tag;
    QString name;
    RuleSetKind kind = RuleSetKind::GeoSite;
    RuleSetFormat preferredFormat = RuleSetFormat::BinarySrs;

    QUrl srsUrl;
    QUrl jsonUrl;
    QUrl checksumUrl;

    QString localSrsPath;
    QString localJsonPath;

    QString sha256;
    QString expectedSha256;

    qint64 sizeBytes = 0;
    QDateTime modifiedAt;
    RuleSetStatus status = RuleSetStatus::Unknown;

    bool builtIn = false;
    bool enabled = true;
    QString description;
    QString lastError;
};

} // namespace zarya
