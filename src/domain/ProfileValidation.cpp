#include "domain/ProfileValidation.h"

#include "domain/ProtocolType.h"

#include <QRegularExpression>

namespace zarya {

namespace {

bool isHexShortId(const QString& value)
{
    if (value.isEmpty()) {
        return true;
    }
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-fA-F]+$"));
    if (!pattern.match(value).hasMatch()) {
        return false;
    }
    return value.length() % 2 == 0 && value.length() <= 16;
}

QString normalizedNetwork(const Profile& profile)
{
    return profile.network.trimmed().isEmpty() ? QStringLiteral("tcp")
                                               : profile.network.trimmed();
}

} // namespace

ProfileValidationResult validateProfileForDialog(const Profile& profile)
{
    if (!profile.isValid()) {
        return {false, QStringLiteral("Name and address are required; port must be 1–65535.")};
    }

    if (profile.uuidPassword.trimmed().isEmpty()) {
        return {false, QStringLiteral("UUID is required.")};
    }

    if (profile.protocol == ProtocolType::Vless && profile.isSecurityReality()) {
        if (profile.publicKey.trimmed().isEmpty()) {
            return {false, QStringLiteral("Public key is required for REALITY.")};
        }
        if (profile.effectiveServerName().isEmpty()) {
            return {false, QStringLiteral("Server name (SNI) is required for REALITY.")};
        }
        const QString network = normalizedNetwork(profile);
        if (network.compare(QStringLiteral("tcp"), Qt::CaseInsensitive) != 0) {
            return {false, QStringLiteral("REALITY with Xray requires network tcp.")};
        }
        if (!isHexShortId(profile.shortId.trimmed())) {
            return {false,
                    QStringLiteral(
                        "Short ID must be even-length hexadecimal (max 16 hex digits), or empty.")};
        }
    }

    if (profile.protocol == ProtocolType::Vless && profile.isSecurityTls()) {
        if (profile.effectiveServerName().isEmpty()) {
            return {false, QStringLiteral("Server name (SNI) is recommended for TLS.")};
        }
    }

    return {true, {}};
}

ProfileValidationResult validateProfileForXray(const Profile& profile)
{
    return validateProfileForDialog(profile);
}

} // namespace zarya
