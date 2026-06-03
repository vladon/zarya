#pragma once

#include "domain/Profile.h"

#include <QString>

namespace zarya {

struct ShareLinkParseResult {
    bool ok = false;
    Profile profile;
    QString error;
};

class ShareLinkParser {
public:
    static ShareLinkParseResult parse(const QString& link);
    static bool isSupportedScheme(const QString& link);
};

} // namespace zarya
