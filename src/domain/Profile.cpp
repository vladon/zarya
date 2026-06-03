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
    profile.enabled = true;
    profile.coreType = CoreType::Xray;
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

} // namespace zarya
