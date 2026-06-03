#include "import/VlessUriParser.h"

#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

namespace zarya {

namespace {

QString decodeComponent(const QString& value)
{
    return QUrl::fromPercentEncoding(value.toUtf8());
}

void applyQueryParam(Profile& profile, const QString& key, const QString& value)
{
    const QString decoded = decodeComponent(value);
    if (key == QStringLiteral("type")) {
        profile.network = decoded;
    } else if (key == QStringLiteral("security")) {
        profile.security = decoded;
    } else if (key == QStringLiteral("pbk")) {
        profile.publicKey = decoded;
    } else if (key == QStringLiteral("fp")) {
        profile.fingerprint = decoded;
    } else if (key == QStringLiteral("sni")) {
        profile.serverName = decoded;
        if (profile.sni.isEmpty()) {
            profile.sni = decoded;
        }
    } else if (key == QStringLiteral("sid")) {
        profile.shortId = decoded;
    } else if (key == QStringLiteral("spx")) {
        profile.spiderX = decoded;
    } else if (key == QStringLiteral("flow")) {
        profile.flow = decoded;
    } else if (key == QStringLiteral("encryption")) {
        profile.encryption = decoded.isEmpty() ? QStringLiteral("none") : decoded;
    } else if (key == QStringLiteral("host")) {
        profile.host = decoded;
    } else if (key == QStringLiteral("path")) {
        profile.path = decoded;
    } else if (key == QStringLiteral("headerType")) {
        profile.headerType = decoded;
    }
}

} // namespace

VlessParseResult VlessUriParser::parseLink(const QString& rawLink)
{
    VlessParseResult result;
    QString link = rawLink.trimmed();
    if (link.isEmpty()) {
        result.errorMessage = QStringLiteral("Empty link.");
        return result;
    }

    if (!link.startsWith(QStringLiteral("vless://"), Qt::CaseInsensitive)) {
        result.errorMessage = QStringLiteral("Not a vless:// link.");
        return result;
    }

    const QUrl url = QUrl(link);
    if (!url.isValid() || url.scheme().compare(QStringLiteral("vless"), Qt::CaseInsensitive) != 0) {
        result.errorMessage = QStringLiteral("Invalid vless URL.");
        return result;
    }

    Profile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.protocol = ProtocolType::Vless;
    profile.coreType = CoreType::Xray;
    profile.enabled = true;
    profile.encryption = QStringLiteral("none");

    profile.uuidPassword = decodeComponent(url.userName());
    profile.address = url.host(QUrl::FullyDecoded);
    profile.port = url.port(443);

    if (profile.uuidPassword.isEmpty()) {
        result.errorMessage = QStringLiteral("Missing UUID in link.");
        return result;
    }
    if (profile.address.isEmpty()) {
        result.errorMessage = QStringLiteral("Missing host in link.");
        return result;
    }
    if (profile.port < 1 || profile.port > 65535) {
        result.errorMessage = QStringLiteral("Invalid port in link.");
        return result;
    }

    const QUrlQuery query(url);
    for (const QPair<QString, QString>& item : query.queryItems()) {
        applyQueryParam(profile, item.first.toLower(), item.second);
    }

    QString name = decodeComponent(url.fragment());
    if (name.isEmpty()) {
        name = QStringLiteral("%1:%2").arg(profile.address).arg(profile.port);
    }
    profile.name = name;

    if (profile.network.isEmpty()) {
        profile.network = QStringLiteral("tcp");
    }
    if (profile.security.isEmpty() && !profile.publicKey.isEmpty()) {
        profile.security = QStringLiteral("reality");
    }

    result.profile = profile;
    result.success = true;
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
