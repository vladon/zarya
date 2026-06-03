#include "subscription/SubscriptionParser.h"

#include "subscription/ShareLinkParser.h"

#include <QRegularExpression>

namespace zarya {

namespace {

QStringList splitLines(const QByteArray& content)
{
    const QString text = QString::fromUtf8(content);
    return text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
}

QByteArray decodeBase64Content(const QByteArray& content)
{
    QString normalized = QString::fromUtf8(content).trimmed();
    normalized.remove(QRegularExpression(QStringLiteral("\\s")));
    const int remainder = normalized.size() % 4;
    if (remainder != 0) {
        normalized.append(QString(4 - remainder, QLatin1Char('=')));
    }
    return QByteArray::fromBase64(normalized.toUtf8());
}

SubscriptionParseResult parseLines(const QStringList& lines)
{
    SubscriptionParseResult result;
    result.success = true;

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        if (!ShareLinkParser::isSupportedScheme(line)) {
            ++result.skippedLines;
            result.warnings.append(
                QStringLiteral("Skipped unsupported line: %1").arg(line.left(80)));
            continue;
        }

        const ShareLinkParseResult parsed = ShareLinkParser::parse(line);
        if (!parsed.ok) {
            ++result.skippedLines;
            result.warnings.append(parsed.error);
            continue;
        }

        result.profiles.append(parsed.profile);
    }

    if (result.profiles.isEmpty() && result.skippedLines > 0) {
        result.success = false;
        result.errorMessage = QStringLiteral("No supported share links found in subscription.");
    }

    return result;
}

} // namespace

bool SubscriptionParser::contentLooksLikePlainShareLinks(const QByteArray& content)
{
    const QStringList lines = splitLines(content);
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (ShareLinkParser::isSupportedScheme(trimmed)) {
            return true;
        }
    }
    return false;
}

SubscriptionParseResult SubscriptionParser::parse(const QByteArray& content)
{
    SubscriptionParseResult result;
    if (content.isEmpty()) {
        result.errorMessage = QStringLiteral("Subscription body is empty.");
        return result;
    }

    if (contentLooksLikePlainShareLinks(content)) {
        return parseLines(splitLines(content));
    }

    const QByteArray decoded = decodeBase64Content(content);
    if (decoded.isEmpty()) {
        result.errorMessage = QStringLiteral("Subscription is not plain share links and base64 decode failed.");
        return result;
    }

    if (contentLooksLikePlainShareLinks(decoded)) {
        return parseLines(splitLines(decoded));
    }

    result.errorMessage =
        QStringLiteral("Subscription format not recognized (expected share links or base64 share links).");
    return result;
}

} // namespace zarya
