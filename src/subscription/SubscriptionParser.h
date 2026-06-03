#pragma once

#include "domain/Profile.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace zarya {

struct SubscriptionParseResult {
    QVector<Profile> profiles;
    QStringList warnings;
    int skippedLines = 0;
    bool success = false;
    QString errorMessage;
};

class SubscriptionParser {
public:
    static SubscriptionParseResult parse(const QByteArray& content);
    static bool contentLooksLikePlainShareLinks(const QByteArray& content);
};

} // namespace zarya
