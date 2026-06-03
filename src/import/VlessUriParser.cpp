#include "import/VlessUriParser.h"

#include "subscription/ShareLinkParser.h"

#include <QRegularExpression>

namespace zarya {

VlessParseResult VlessUriParser::parseLink(const QString& rawLink)
{
    VlessParseResult result;
    const ShareLinkParseResult parsed = ShareLinkParser::parse(rawLink);
    result.success = parsed.ok;
    result.profile = parsed.profile;
    result.errorMessage = parsed.error;
    return result;
}

QVector<VlessParseResult> VlessUriParser::parseMany(const QString& text)
{
    QVector<VlessParseResult> results;
    const QStringList lines =
        text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        results.append(parseLink(trimmed));
    }
    return results;
}

} // namespace zarya
