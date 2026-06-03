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
    QString network = profile.network.trimmed().toLower();
    if (network.isEmpty() || network == QStringLiteral("raw")) {
        return QStringLiteral("tcp");
    }
    return network;
}

ProfileValidationResult validateVlessDialog(const Profile& profile)
{
    if (profile.effectiveUuid().isEmpty()) {
        return {false, QStringLiteral("UUID is required for VLESS.")};
    }

    if (profile.isSecurityReality()) {
        if (profile.publicKey.trimmed().isEmpty()) {
            return {false, QStringLiteral("Public key is required for REALITY.")};
        }
        if (profile.effectiveServerName().isEmpty()) {
            return {false, QStringLiteral("Server name (SNI) is required for REALITY.")};
        }
        if (normalizedNetwork(profile) != QStringLiteral("tcp")) {
            return {false, QStringLiteral("REALITY with Xray requires network tcp.")};
        }
        if (!isHexShortId(profile.shortId.trimmed())) {
            return {false,
                    QStringLiteral(
                        "Short ID must be even-length hexadecimal (max 16 hex digits), or empty.")};
        }
    }

    if (profile.isSecurityTls() && profile.effectiveServerName().isEmpty()) {
        return {false, QStringLiteral("Server name (SNI) is recommended for TLS.")};
    }

    return {true, {}};
}

ProfileValidationResult validateVmessDialog(const Profile& profile)
{
    if (profile.effectiveUuid().isEmpty()) {
        return {false, QStringLiteral("UUID is required for VMess.")};
    }
    if (profile.alterId < 0) {
        return {false, QStringLiteral("Alter ID must be >= 0.")};
    }
    return {true, {}};
}

ProfileValidationResult validateTrojanDialog(const Profile& profile)
{
    if (profile.effectivePassword().isEmpty()) {
        return {false, QStringLiteral("Password is required for Trojan.")};
    }
    return {true, {}};
}

ProfileValidationResult validateShadowsocksDialog(const Profile& profile)
{
    if (profile.hasUnsupportedFeature()) {
        return {false, profile.unsupportedReason};
    }
    if (profile.effectiveMethod().isEmpty()) {
        return {false, QStringLiteral("Method is required for Shadowsocks.")};
    }
    if (profile.effectivePassword().isEmpty()) {
        return {false, QStringLiteral("Password is required for Shadowsocks.")};
    }
    return {true, {}};
}

ProfileValidationResult validateSocksDialog(const Profile& profile)
{
    Q_UNUSED(profile);
    return {true, {}};
}

} // namespace

ProfileValidationResult validateProfileForDialog(const Profile& profile)
{
    if (!profile.isValid()) {
        return {false, QStringLiteral("Name and address are required; port must be 1–65535.")};
    }

    switch (profile.protocol) {
    case ProtocolType::Vless:
        return validateVlessDialog(profile);
    case ProtocolType::Vmess:
        return validateVmessDialog(profile);
    case ProtocolType::Trojan:
        return validateTrojanDialog(profile);
    case ProtocolType::Shadowsocks:
        return validateShadowsocksDialog(profile);
    case ProtocolType::Socks:
        return validateSocksDialog(profile);
    }

    return {true, {}};
}

ProfileValidationResult validateProfileForXray(const Profile& profile)
{
    return validateProfileForDialog(profile);
}

} // namespace zarya
