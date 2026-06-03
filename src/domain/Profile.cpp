#include "domain/Profile.h"

#include <QUuid>

namespace zarya {

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

} // namespace zarya
