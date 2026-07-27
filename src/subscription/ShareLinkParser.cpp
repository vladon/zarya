#include "subscription/ShareLinkParser.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
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

QByteArray decodeBase64Flexible(const QString& input)
{
    QString normalized = input.trimmed();
    normalized.remove(QRegularExpression(QStringLiteral("\\s")));
    const int remainder = normalized.size() % 4;
    if (remainder != 0) {
        normalized.append(QString(4 - remainder, QLatin1Char('=')));
    }
    return QByteArray::fromBase64(normalized.toUtf8());
}

void applyShareLinkQueryParam(Profile& profile, const QString& key, const QString& value)
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
        profile.sni = decoded;
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
    } else if (key == QStringLiteral("alpn")) {
        profile.alpn = decoded;
    } else if (key == QStringLiteral("obfs")) {
        profile.obfs = decoded;
    } else if (key == QStringLiteral("obfs-password") || key == QStringLiteral("obfspassword")) {
        profile.obfsPassword = decoded;
    } else if (key == QStringLiteral("auth") || key == QStringLiteral("password")) {
        // Used by some hysteria2 share links when auth is not in userinfo.
        if (profile.password.isEmpty()) {
            profile.password = decoded;
            profile.uuidPassword = decoded;
        }
    } else if (key == QStringLiteral("publickey") || key == QStringLiteral("peerpublickey")) {
        profile.publicKey = decoded;
    } else if (key == QStringLiteral("address") || key == QStringLiteral("localaddress")
               || key == QStringLiteral("ip")) {
        // WireGuard local interface address(es); do not overwrite server host.
        if (profile.protocol == ProtocolType::WireGuard) {
            profile.localAddress = decoded;
        }
    } else if (key == QStringLiteral("mtu")) {
        bool ok = false;
        const int mtu = decoded.toInt(&ok);
        if (ok && mtu > 0) {
            profile.mtu = mtu;
        }
    } else if (key == QStringLiteral("reserved")) {
        profile.reserved = decoded;
    } else if (key == QStringLiteral("presharedkey") || key == QStringLiteral("psk")) {
        profile.preSharedKey = decoded;
    } else if (key == QStringLiteral("allowedips") || key == QStringLiteral("allowedip")) {
        profile.allowedIps = decoded;
    } else if (key == QStringLiteral("keepalive") || key == QStringLiteral("persistentkeepalive")) {
        bool ok = false;
        const int keepAlive = decoded.toInt(&ok);
        if (ok && keepAlive >= 0) {
            profile.keepAlive = keepAlive;
        }
    } else if (key == QStringLiteral("privatekey") || key == QStringLiteral("secretkey")) {
        if (profile.password.isEmpty()) {
            profile.password = decoded;
            profile.uuidPassword = decoded;
        }
    } else if (key == QStringLiteral("allowinsecure") || key == QStringLiteral("insecure")) {
        profile.allowInsecure =
            decoded.compare(QStringLiteral("1")) == 0
            || decoded.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
    }
}

Profile baseImportedProfile(ProtocolType protocol)
{
    Profile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.protocol = protocol;
    profile.coreType = CoreType::Xray;
    profile.enabled = true;
    profile.encryption = QStringLiteral("none");
    profile.network = QStringLiteral("tcp");
    return profile;
}

ShareLinkParseResult parseVless(const QString& link)
{
    ShareLinkParseResult result;
    const QUrl url = QUrl(link);
    if (!url.isValid() || url.scheme().compare(QStringLiteral("vless"), Qt::CaseInsensitive) != 0) {
        result.error = QStringLiteral("Invalid vless URL.");
        return result;
    }

    Profile profile = baseImportedProfile(ProtocolType::Vless);
    profile.uuidPassword = decodeComponent(url.userName());
    profile.address = url.host(QUrl::FullyDecoded);
    profile.port = url.port(443);

    if (profile.uuidPassword.isEmpty()) {
        result.error = QStringLiteral("Missing UUID in vless link.");
        return result;
    }
    if (profile.address.isEmpty() || profile.port < 1 || profile.port > 65535) {
        result.error = QStringLiteral("Invalid host or port in vless link.");
        return result;
    }

    const QUrlQuery query(url);
    for (const QPair<QString, QString>& item : query.queryItems()) {
        applyShareLinkQueryParam(profile, item.first.toLower(), item.second);
    }

    QString name = decodeComponent(url.fragment());
    profile.name = name.isEmpty()
                       ? QStringLiteral("%1:%2").arg(profile.address).arg(profile.port)
                       : name;

    if (profile.security.isEmpty() && !profile.publicKey.isEmpty()) {
        profile.security = QStringLiteral("reality");
    }

    result.profile = profile;
    result.ok = true;
    return result;
}

ShareLinkParseResult parseVmess(const QString& link)
{
    ShareLinkParseResult result;
    const QString payload = link.mid(QStringLiteral("vmess://").size());
    const QByteArray jsonBytes = decodeBase64Flexible(payload);
    if (jsonBytes.isEmpty()) {
        result.error = QStringLiteral("Invalid vmess base64 payload.");
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.error = QStringLiteral("Invalid vmess JSON.");
        return result;
    }

    const QJsonObject object = document.object();
    Profile profile = baseImportedProfile(ProtocolType::Vmess);
    profile.name = object.value(QStringLiteral("ps")).toString();
    profile.address = object.value(QStringLiteral("add")).toString();
    profile.port = object.value(QStringLiteral("port")).toInt(443);
    profile.uuidPassword = object.value(QStringLiteral("id")).toString();
    profile.alterId = object.value(QStringLiteral("aid")).toInt(0);
    profile.network = object.value(QStringLiteral("net")).toString(QStringLiteral("tcp"));
    profile.headerType = object.value(QStringLiteral("type")).toString();
    profile.host = object.value(QStringLiteral("host")).toString();
    profile.path = object.value(QStringLiteral("path")).toString();

    const QString tls = object.value(QStringLiteral("tls")).toString();
    profile.security = tls.isEmpty() ? QStringLiteral("none") : tls;
    profile.serverName = object.value(QStringLiteral("sni")).toString();
    profile.sni = profile.serverName;
    profile.fingerprint = object.value(QStringLiteral("fp")).toString();

    const QString scy = object.value(QStringLiteral("scy")).toString();
    profile.securityCipher = scy.isEmpty() ? QStringLiteral("auto") : scy;

    if (profile.serverName.isEmpty() && profile.isSecurityTls()) {
        if (!profile.host.isEmpty()) {
            profile.serverName = profile.host;
        } else {
            profile.serverName = profile.address;
        }
        profile.sni = profile.serverName;
    }

    if (profile.name.isEmpty()) {
        profile.name = QStringLiteral("%1:%2").arg(profile.address).arg(profile.port);
    }
    if (profile.address.isEmpty() || profile.uuidPassword.isEmpty()) {
        result.error = QStringLiteral("Incomplete vmess profile fields.");
        return result;
    }

    result.profile = profile;
    result.ok = true;
    return result;
}

ShareLinkParseResult parseTrojan(const QString& link)
{
    ShareLinkParseResult result;
    const QUrl url = QUrl(link);
    if (!url.isValid() || url.scheme().compare(QStringLiteral("trojan"), Qt::CaseInsensitive) != 0) {
        result.error = QStringLiteral("Invalid trojan URL.");
        return result;
    }

    Profile profile = baseImportedProfile(ProtocolType::Trojan);
    const QString trojanPassword = decodeComponent(url.userName());
    profile.password = trojanPassword;
    profile.uuidPassword = trojanPassword;
    profile.address = url.host(QUrl::FullyDecoded);
    profile.port = url.port(443);

    if (profile.password.isEmpty() || profile.address.isEmpty()) {
        result.error = QStringLiteral("Incomplete trojan link.");
        return result;
    }

    const QUrlQuery query(url);
    for (const QPair<QString, QString>& item : query.queryItems()) {
        applyShareLinkQueryParam(profile, item.first.toLower(), item.second);
    }

    if (profile.security.isEmpty() && !profile.publicKey.isEmpty()) {
        profile.security = QStringLiteral("reality");
    }
    if (profile.security.isEmpty() && profile.port == 443) {
        profile.security = QStringLiteral("tls");
    }

    QString name = decodeComponent(url.fragment());
    profile.name = name.isEmpty()
                       ? QStringLiteral("%1:%2").arg(profile.address).arg(profile.port)
                       : name;

    result.profile = profile;
    result.ok = true;
    return result;
}

bool parseShadowsocksUserInfo(const QString& userInfo, QString* method, QString* password)
{
    const int colon = userInfo.indexOf(QLatin1Char(':'));
    if (colon <= 0) {
        return false;
    }
    *method = decodeComponent(userInfo.left(colon));
    *password = decodeComponent(userInfo.mid(colon + 1));
    return !method->isEmpty();
}

// SIP002 host part may be "host:port", "host:port/", or "host:port?plugin=...&type=tcp".
bool parseShadowsocksHostPort(QString hostPart, QString* host, int* port)
{
    const int queryIndex = hostPart.indexOf(QLatin1Char('?'));
    if (queryIndex >= 0) {
        hostPart = hostPart.left(queryIndex);
    }
    const int slashIndex = hostPart.indexOf(QLatin1Char('/'));
    if (slashIndex >= 0) {
        hostPart = hostPart.left(slashIndex);
    }
    hostPart = hostPart.trimmed();
    if (hostPart.isEmpty()) {
        return false;
    }

    // Prefer bracketed IPv6: [2001:db8::1]:8388
    if (hostPart.startsWith(QLatin1Char('['))) {
        const int close = hostPart.indexOf(QLatin1Char(']'));
        if (close <= 1) {
            return false;
        }
        *host = hostPart.mid(1, close - 1);
        if (hostPart.size() > close + 1 && hostPart.at(close + 1) == QLatin1Char(':')) {
            bool ok = false;
            *port = hostPart.mid(close + 2).toInt(&ok);
            return ok && *port >= 1 && *port <= 65535;
        }
        return false;
    }

    const int colon = hostPart.lastIndexOf(QLatin1Char(':'));
    if (colon <= 0 || colon + 1 >= hostPart.size()) {
        return false;
    }
    *host = hostPart.left(colon);
    bool ok = false;
    *port = hostPart.mid(colon + 1).toInt(&ok);
    return ok && *port >= 1 && *port <= 65535 && !host->isEmpty();
}

ShareLinkParseResult parseShadowsocks(const QString& link)
{
    ShareLinkParseResult result;
    QString remainder = link.mid(QStringLiteral("ss://").size());

    QString name;
    const int hashIndex = remainder.indexOf(QLatin1Char('#'));
    if (hashIndex >= 0) {
        name = decodeComponent(remainder.mid(hashIndex + 1));
        remainder = remainder.left(hashIndex);
    }

    Profile profile = baseImportedProfile(ProtocolType::Shadowsocks);
    QString method;
    QString password;
    QString host;
    int port = 0;

    if (remainder.contains(QLatin1Char('@'))) {
        const int at = remainder.indexOf(QLatin1Char('@'));
        const QString userPart = remainder.left(at);
        const QString hostPart = remainder.mid(at + 1);
        QString decodedUser = decodeComponent(userPart);
        if (!decodedUser.contains(QLatin1Char(':'))) {
            const QByteArray decoded = decodeBase64Flexible(userPart);
            decodedUser = QString::fromUtf8(decoded);
        }
        if (!parseShadowsocksUserInfo(decodedUser, &method, &password)) {
            result.error = QStringLiteral("Invalid shadowsocks credentials.");
            return result;
        }

        if (!parseShadowsocksHostPort(hostPart, &host, &port)) {
            result.error = QStringLiteral("Invalid shadowsocks host or port.");
            return result;
        }
    } else {
        const QByteArray decoded = decodeBase64Flexible(remainder);
        const QString decodedText = QString::fromUtf8(decoded);
        const int at = decodedText.indexOf(QLatin1Char('@'));
        if (at > 0) {
            if (!parseShadowsocksUserInfo(decodedText.left(at), &method, &password)) {
                result.error = QStringLiteral("Invalid shadowsocks credentials.");
                return result;
            }
            if (!parseShadowsocksHostPort(decodedText.mid(at + 1), &host, &port)) {
                result.error = QStringLiteral("Invalid shadowsocks host or port.");
                return result;
            }
        } else if (!parseShadowsocksUserInfo(decodedText, &method, &password)) {
            result.error = QStringLiteral("Unsupported shadowsocks link format.");
            return result;
        }
    }

    if (host.isEmpty() || method.isEmpty() || port < 1 || port > 65535) {
        result.error = QStringLiteral("Incomplete shadowsocks link.");
        return result;
    }

    profile.address = host;
    profile.port = port;
    profile.password = password;
    profile.uuidPassword = password;
    profile.method = method;
    profile.encryption = method;

    QString hostPartForQuery = remainder;
    if (remainder.contains(QLatin1Char('@'))) {
        hostPartForQuery = remainder.mid(remainder.indexOf(QLatin1Char('@')) + 1);
    }
    const int queryIndex = hostPartForQuery.indexOf(QLatin1Char('?'));
    if (queryIndex >= 0) {
        const QUrlQuery pluginQuery(hostPartForQuery.mid(queryIndex + 1));
        if (!pluginQuery.queryItemValue(QStringLiteral("plugin")).isEmpty()) {
            profile.unsupportedReason =
                QStringLiteral("Shadowsocks plugin options are not supported yet");
        }
        const QString network = pluginQuery.queryItemValue(QStringLiteral("type")).trimmed();
        if (!network.isEmpty()) {
            profile.network = network;
        }
    }

    profile.name = name.isEmpty() ? QStringLiteral("%1:%2").arg(host).arg(port) : name;

    result.profile = profile;
    result.ok = true;
    return result;
}

ShareLinkParseResult parseHysteria2(const QString& link)
{
    ShareLinkParseResult result;
    const QUrl url = QUrl(link);
    const QString scheme = url.scheme().toLower();
    if (!url.isValid()
        || (scheme != QStringLiteral("hysteria2") && scheme != QStringLiteral("hy2"))) {
        result.error = QStringLiteral("Invalid hysteria2 URL.");
        return result;
    }

    Profile profile = baseImportedProfile(ProtocolType::Hysteria2);
    profile.network = QStringLiteral("hysteria");
    const QString auth = decodeComponent(url.userName());
    if (!auth.isEmpty()) {
        profile.password = auth;
        profile.uuidPassword = auth;
    }
    profile.address = url.host(QUrl::FullyDecoded);
    profile.port = url.port(443);

    if (profile.address.isEmpty() || profile.port < 1 || profile.port > 65535) {
        result.error = QStringLiteral("Invalid host or port in hysteria2 link.");
        return result;
    }

    const QUrlQuery query(url);
    for (const QPair<QString, QString>& item : query.queryItems()) {
        applyShareLinkQueryParam(profile, item.first.toLower(), item.second);
    }

    if (profile.password.isEmpty()) {
        result.error = QStringLiteral("Incomplete hysteria2 link (missing auth).");
        return result;
    }

    if (profile.security.isEmpty()) {
        profile.security = QStringLiteral("tls");
    }

    const QString obfs = profile.obfs.trimmed().toLower();
    if (!obfs.isEmpty() && obfs != QStringLiteral("salamander") && obfs != QStringLiteral("none")) {
        profile.unsupportedReason =
            QStringLiteral("Hysteria2 obfuscation type is not supported: %1").arg(profile.obfs);
    }
    if (obfs == QStringLiteral("salamander") && profile.obfsPassword.trimmed().isEmpty()) {
        profile.unsupportedReason =
            QStringLiteral("Hysteria2 salamander obfuscation requires obfs-password.");
    }

    QString name = decodeComponent(url.fragment());
    profile.name = name.isEmpty()
                       ? QStringLiteral("%1:%2").arg(profile.address).arg(profile.port)
                       : name;

    result.profile = profile;
    result.ok = true;
    return result;
}

ShareLinkParseResult parseWireGuard(const QString& link)
{
    ShareLinkParseResult result;
    const QUrl url = QUrl(link);
    const QString scheme = url.scheme().toLower();
    if (!url.isValid()
        || (scheme != QStringLiteral("wireguard") && scheme != QStringLiteral("wg"))) {
        result.error = QStringLiteral("Invalid wireguard URL.");
        return result;
    }

    Profile profile = baseImportedProfile(ProtocolType::WireGuard);
    profile.network = QStringLiteral("udp");
    const QString privateKey = decodeComponent(url.userName());
    if (!privateKey.isEmpty()) {
        profile.password = privateKey;
        profile.uuidPassword = privateKey;
    }
    profile.address = url.host(QUrl::FullyDecoded);
    profile.port = url.port(51820);

    if (profile.address.isEmpty() || profile.port < 1 || profile.port > 65535) {
        result.error = QStringLiteral("Invalid host or port in wireguard link.");
        return result;
    }

    const QUrlQuery query(url);
    for (const QPair<QString, QString>& item : query.queryItems()) {
        applyShareLinkQueryParam(profile, item.first.toLower(), item.second);
    }

    if (profile.password.isEmpty()) {
        result.error = QStringLiteral("Incomplete wireguard link (missing private key).");
        return result;
    }
    if (profile.publicKey.trimmed().isEmpty()) {
        result.error = QStringLiteral("Incomplete wireguard link (missing peer public key).");
        return result;
    }

    QString name = decodeComponent(url.fragment());
    profile.name = name.isEmpty()
                       ? QStringLiteral("%1:%2").arg(profile.address).arg(profile.port)
                       : name;

    result.profile = profile;
    result.ok = true;
    return result;
}

} // namespace

bool ShareLinkParser::isSupportedScheme(const QString& link)
{
    const QString trimmed = link.trimmed();
    return trimmed.startsWith(QStringLiteral("vless://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("vmess://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("trojan://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("ss://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("socks://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("hysteria2://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("hy2://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("wireguard://"), Qt::CaseInsensitive)
           || trimmed.startsWith(QStringLiteral("wg://"), Qt::CaseInsensitive);
}

ShareLinkParseResult ShareLinkParser::parse(const QString& link)
{
    ShareLinkParseResult result;
    const QString trimmed = link.trimmed();
    if (trimmed.isEmpty()) {
        result.error = QStringLiteral("Empty link.");
        return result;
    }

    if (trimmed.startsWith(QStringLiteral("vless://"), Qt::CaseInsensitive)) {
        return parseVless(trimmed);
    }
    if (trimmed.startsWith(QStringLiteral("vmess://"), Qt::CaseInsensitive)) {
        return parseVmess(trimmed);
    }
    if (trimmed.startsWith(QStringLiteral("trojan://"), Qt::CaseInsensitive)) {
        return parseTrojan(trimmed);
    }
    if (trimmed.startsWith(QStringLiteral("ss://"), Qt::CaseInsensitive)) {
        return parseShadowsocks(trimmed);
    }
    if (trimmed.startsWith(QStringLiteral("hysteria2://"), Qt::CaseInsensitive)
        || trimmed.startsWith(QStringLiteral("hy2://"), Qt::CaseInsensitive)) {
        return parseHysteria2(trimmed);
    }
    if (trimmed.startsWith(QStringLiteral("wireguard://"), Qt::CaseInsensitive)
        || trimmed.startsWith(QStringLiteral("wg://"), Qt::CaseInsensitive)) {
        return parseWireGuard(trimmed);
    }

    result.error = QStringLiteral("Unsupported share link scheme.");
    return result;
}

} // namespace zarya
