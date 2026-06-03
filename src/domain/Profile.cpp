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

} // namespace zarya
