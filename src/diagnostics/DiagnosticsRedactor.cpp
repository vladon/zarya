#include "diagnostics/DiagnosticsRedactor.h"

#include "domain/Profile.h"
#include "domain/ProfileSourceType.h"
#include "domain/ProtocolType.h"

#include <QJsonArray>
#include <QRegularExpression>

namespace zarya {

QStringList DiagnosticsRedactor::s_redactedFields;

void DiagnosticsRedactor::resetReport()
{
    s_redactedFields.clear();
}

QStringList DiagnosticsRedactor::redactedFieldReport()
{
    return s_redactedFields;
}

void DiagnosticsRedactor::trackField(const QString& field)
{
    if (!s_redactedFields.contains(field)) {
        s_redactedFields.append(field);
    }
}

namespace {

constexpr auto kRedacted = "<redacted>";
constexpr auto kRedactedHost = "<redacted-host>";
constexpr auto kRedactedUrl = "<redacted-url>";
constexpr auto kRedactedUser = "<user>";

void track(const QString& field)
{
    DiagnosticsRedactor::trackField(field);
}

QString redactPathInternal(const QString& path, DiagnosticsRedactionMode mode, bool includeMachinePaths)
{
    if (path.isEmpty()) {
        return path;
    }
    if (includeMachinePaths && mode == DiagnosticsRedactionMode::Basic) {
        return path;
    }

    QString redacted = path;
    static const QRegularExpression winUserPath(
        QStringLiteral(R"(^([A-Za-z]:\\Users\\)[^\\]+(\\.*)?$)"));
    static const QRegularExpression macUserPath(QStringLiteral(R"(^(/Users/)[^/]+(/.*)?$)"));
    static const QRegularExpression linuxHomePath(QStringLiteral(R"(^(/home/)[^/]+(/.*)?$)"));

    auto replaceUser = [&](const QRegularExpression& pattern, const QString& prefix) {
        const QRegularExpressionMatch match = pattern.match(redacted);
        if (match.hasMatch()) {
            const QString suffix = match.captured(2);
            redacted = prefix + kRedactedUser + (suffix.isEmpty() ? QString() : suffix);
            track(QStringLiteral("paths.username"));
        }
    };

    replaceUser(winUserPath, QStringLiteral("C:\\Users\\"));
    replaceUser(macUserPath, QStringLiteral("/Users/"));
    replaceUser(linuxHomePath, QStringLiteral("/home/"));

    if (mode == DiagnosticsRedactionMode::Strict && !includeMachinePaths) {
        static const QRegularExpression absPath(QStringLiteral(R"(([A-Za-z]:\\|/)[^\s"]+)"));
        redacted.replace(absPath, QStringLiteral("<redacted-path>"));
    }
    return redacted;
}

QString redactShareLinks(const QString& text)
{
    static const QRegularExpression shareLink(
        QStringLiteral(R"((vless|vmess|trojan|ss)://[^\s"']+)"),
        QRegularExpression::CaseInsensitiveOption);
    QString result = text;
    result.replace(shareLink, kRedactedUrl);
    return result;
}

QString redactSensitiveKeysInText(const QString& text, DiagnosticsRedactionMode mode)
{
    QString result = redactShareLinks(text);
    static const QRegularExpression uuidPattern(
        QStringLiteral(R"(\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\b)"));
    result.replace(uuidPattern, kRedacted);

    if (result.contains(QStringLiteral("helper.token"), Qt::CaseInsensitive)) {
        result.replace(QRegularExpression(QStringLiteral(R"(helper\.token[^\n]*)")),
                       QStringLiteral("helper.token <redacted>"));
    }

    if (mode == DiagnosticsRedactionMode::Strict) {
        static const QRegularExpression hostPattern(
            QStringLiteral(R"((https?://)?[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"));
        result.replace(hostPattern, kRedactedHost);
    }
    return result;
}

bool isSensitiveJsonKey(const QString& key)
{
    const QString lower = key.toLower();
    return lower.contains(QStringLiteral("uuid")) || lower.contains(QStringLiteral("password"))
           || lower.contains(QStringLiteral("publickey")) || lower.contains(QStringLiteral("shortid"))
           || lower.contains(QStringLiteral("token")) || lower == QStringLiteral("url")
           || lower.contains(QStringLiteral("servername")) || lower == QStringLiteral("sni")
           || lower == QStringLiteral("address") || lower == QStringLiteral("host");
}

QJsonValue redactJsonValue(const QJsonValue& value, DiagnosticsRedactionMode mode,
                           const QString& keyPath)
{
    if (value.isObject()) {
        return DiagnosticsRedactor::redactJsonObject(value.toObject(), mode);
    }
    if (value.isArray()) {
        QJsonArray array;
        for (const QJsonValue& item : value.toArray()) {
            array.append(redactJsonValue(item, mode, keyPath));
        }
        return array;
    }
    if (!value.isString()) {
        return value;
    }

    const QString keyLower = keyPath.toLower();
    if (keyLower.contains(QStringLiteral("token")) || keyLower.contains(QStringLiteral("password"))
        || keyLower.contains(QStringLiteral("uuid")) || keyLower.contains(QStringLiteral("publickey"))
        || keyLower.contains(QStringLiteral("shortid")) || keyLower == QStringLiteral("url")) {
        track(keyPath);
        return QStringLiteral("<redacted>");
    }
    if (mode == DiagnosticsRedactionMode::Strict
        && (keyLower == QStringLiteral("address") || keyLower == QStringLiteral("servername")
            || keyLower == QStringLiteral("sni") || keyLower == QStringLiteral("host"))) {
        track(keyPath);
        return kRedactedHost;
    }
    return redactSensitiveKeysInText(value.toString(), mode);
}

} // namespace

QString DiagnosticsRedactor::redactPath(const QString& path, DiagnosticsRedactionMode mode,
                                        bool includeMachinePaths)
{
    return redactPathInternal(path, mode, includeMachinePaths);
}

QString DiagnosticsRedactor::redactText(const QString& text, DiagnosticsRedactionMode mode)
{
    return redactSensitiveKeysInText(text, mode);
}

QString DiagnosticsRedactor::redactLogLine(const QString& line, DiagnosticsRedactionMode mode)
{
    QString result = redactSensitiveKeysInText(line, mode);
    result = redactPathInternal(result, mode, false);
    return result;
}

QJsonObject DiagnosticsRedactor::redactJsonObject(const QJsonObject& object,
                                                  DiagnosticsRedactionMode mode)
{
    QJsonObject result;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        const QString key = it.key();
        if (isSensitiveJsonKey(key)) {
            if (key.compare(QStringLiteral("port"), Qt::CaseInsensitive) == 0) {
                result.insert(key, it.value());
            } else if (mode == DiagnosticsRedactionMode::Basic
                       && (key.compare(QStringLiteral("address"), Qt::CaseInsensitive) == 0
                           || key.compare(QStringLiteral("host"), Qt::CaseInsensitive) == 0)) {
                result.insert(key, it.value());
            } else {
                track(key);
                result.insert(key, key.contains(QStringLiteral("url"), Qt::CaseInsensitive)
                                         ? kRedactedUrl
                                         : (key.contains(QStringLiteral("address"))
                                                || key.contains(QStringLiteral("server"))
                                                || key == QStringLiteral("sni")
                                            ? kRedactedHost
                                            : kRedacted));
            }
        } else {
            result.insert(key, redactJsonValue(it.value(), mode, key));
        }
    }
    return result;
}

QJsonObject DiagnosticsRedactor::redactProfileSummary(const Profile& profile,
                                                      DiagnosticsRedactionMode mode)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), profile.id);
    object.insert(QStringLiteral("name"), profile.name);
    object.insert(QStringLiteral("protocol"), protocolTypeToString(profile.protocol));
    object.insert(QStringLiteral("address"),
                  mode == DiagnosticsRedactionMode::Strict ? kRedactedHost : profile.address);
    object.insert(QStringLiteral("port"), profile.port);
    object.insert(QStringLiteral("security"), profile.security);
    object.insert(QStringLiteral("network"), profile.network);
    object.insert(QStringLiteral("hasUuid"), !profile.uuidPassword.isEmpty());
    object.insert(QStringLiteral("hasPassword"), !profile.password.isEmpty());
    object.insert(QStringLiteral("hasPublicKey"), !profile.publicKey.isEmpty());
    object.insert(QStringLiteral("sourceType"), profileSourceTypeToString(profile.sourceType));
    object.insert(QStringLiteral("enabled"), profile.enabled);
    return object;
}

} // namespace zarya
