#pragma once

#include "domain/Profile.h"

#include <QString>
#include <QVector>

namespace zarya {

struct VlessParseResult {
    bool success = false;
    Profile profile;
    QString errorMessage;
};

class VlessUriParser {
public:
    static VlessParseResult parseLink(const QString& link);
    static QVector<VlessParseResult> parseMany(const QString& text);
};

} // namespace zarya
