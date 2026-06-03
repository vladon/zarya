#include "domain/Profile.h"

#include <QUuid>

namespace zarya {

namespace {

bool equalsIgnoreCase(const QString& a, const QString& b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

} // namespace

Profile Profile::createDefault()
{
    Profile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = QStringLiteral("New profile");
    profile.protocol = ProtocolType::Vless;
    profile.address = QStringLiteral("example.com");
    profile.port = 443;
    profile.uuidPassword = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.security = QStringLiteral("tls");
    profile.network = QStringLiteral("tcp");
    profile.encryption = QStringLiteral("none");
    profile.coreType = CoreType::Xray;
    profile.enabled = true;
    return profile;
}

Profile Profile::createVlessRealityDefault()
{
    Profile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = QStringLiteral("VLESS REALITY");
    profile.protocol = ProtocolType::Vless;
    profile.coreType = CoreType::Xray;
    profile.address = QStringLiteral("example.com");
    profile.port = 443;
    profile.uuidPassword = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.encryption = QStringLiteral("none");
    profile.network = QStringLiteral("tcp");
    profile.security = QStringLiteral("reality");
    profile.flow = QStringLiteral("xtls-rprx-vision");
    profile.fingerprint = QStringLiteral("chrome");
    profile.spiderX = QStringLiteral("/");
    profile.enabled = true;
    return profile;
}

bool Profile::isValid() const
{
    return !name.trimmed().isEmpty() && !address.trimmed().isEmpty()
           && port >= 1 && port <= 65535;
}

bool Profile::isSecurityReality() const
{
    return equalsIgnoreCase(security, QStringLiteral("reality"));
}

bool Profile::isSecurityTls() const
{
    return equalsIgnoreCase(security, QStringLiteral("tls"));
}

bool Profile::isSecurityNone() const
{
    const QString value = security.trimmed();
    return value.isEmpty() || equalsIgnoreCase(value, QStringLiteral("none"));
}

QString Profile::effectiveUuid() const
{
    return uuidPassword.trimmed();
}

QString Profile::effectivePassword() const
{
    if (!password.trimmed().isEmpty()) {
        return password.trimmed();
    }
    return uuidPassword.trimmed();
}

QString Profile::effectiveVmessSecurity() const
{
    if (!securityCipher.trimmed().isEmpty()) {
        return securityCipher.trimmed();
    }
    return QStringLiteral("auto");
}

QString Profile::effectiveMethod() const
{
    if (!method.trimmed().isEmpty()) {
        return method.trimmed();
    }
    return encryption.trimmed();
}

bool Profile::hasUnsupportedFeature() const
{
    return !unsupportedReason.trimmed().isEmpty();
}

QString Profile::effectiveServerName() const
{
    if (!serverName.trimmed().isEmpty()) {
        return serverName.trimmed();
    }
    if (!sni.trimmed().isEmpty()) {
        return sni.trimmed();
    }
    return {};
}

QString Profile::effectiveEncryption() const
{
    return encryption.trimmed().isEmpty() ? QStringLiteral("none") : encryption.trimmed();
}

QString Profile::effectiveFingerprint() const
{
    return fingerprint.trimmed().isEmpty() ? QStringLiteral("chrome") : fingerprint.trimmed();
}

QString Profile::computeSourceKey() const
{
    return QStringLiteral("%1|%2|%3|%4|%5|%6")
        .arg(protocolTypeToString(protocol), address.trimmed())
        .arg(port)
        .arg(uuidPassword.trimmed(), effectiveServerName(), security.trimmed());
}

bool Profile::isManual() const
{
    return sourceType == ProfileSourceType::Manual;
}

bool Profile::isFromSubscription(const QString& subId) const
{
    return sourceType == ProfileSourceType::Subscription && subscriptionId == subId;
}

} // namespace zarya
