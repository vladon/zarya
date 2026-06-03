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
        return {false, QStringLiteral("UUID / password cannot be empty.")};
    }

    if (profile.protocol == ProtocolType::Vless && profile.isSecurityReality()) {
        if (profile.publicKey.trimmed().isEmpty()) {
            return {false, QStringLiteral("Public key is required for REALITY.")};
        }
        if (profile.effectiveServerName().isEmpty()) {
            return {false,
                    QStringLiteral("Server name (or SNI) is required for REALITY.")};
        }
        const QString network = normalizedNetwork(profile);
        if (!network.isEmpty() && network.compare(QStringLiteral("tcp"), Qt::CaseInsensitive) != 0) {
            return {false, QStringLiteral("REALITY with Xray currently requires network tcp.")};
        }
        if (!isHexShortId(profile.shortId.trimmed())) {
            return {false,
                    QStringLiteral(
                        "Short ID must be even-length hexadecimal (max 16 hex digits), or empty.")};
        }
        const QString flow = profile.flow.trimmed();
        if (!flow.isEmpty()
            && flow.compare(QStringLiteral("xtls-rprx-vision"), Qt::CaseInsensitive) != 0) {
            return {false,
                    QStringLiteral(
                        "For REALITY vision, flow must be xtls-rprx-vision or left empty.")};
        }
    }

    if (profile.protocol == ProtocolType::Vless && profile.isSecurityTls()) {
        if (profile.effectiveServerName().isEmpty()) {
            return {false, QStringLiteral("Server name (or SNI) is recommended for TLS.")};
        }
    }

    return {true, {}};
}

ProfileValidationResult validateProfileForXray(const Profile& profile)
{
    const ProfileValidationResult dialogResult = validateProfileForDialog(profile);
    if (!dialogResult.ok) {
        return dialogResult;
    }

    if (profile.protocol != ProtocolType::Vless) {
        return {true, {}};
    }

    if (profile.isSecurityReality()) {
        return {true, {}};
    }

    return {true, {}};
}

} // namespace zarya
