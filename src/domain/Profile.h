#pragma once

#include "domain/CoreType.h"
#include "domain/ProfileSourceType.h"
#include "domain/ProtocolType.h"

#include <QDateTime>
#include <QString>

namespace zarya {

struct Profile {
    QString id;
    QString name;
    ProtocolType protocol = ProtocolType::Vless;
    CoreType coreType = CoreType::Xray;
    QString address;
    int port = 443;
    QString uuidPassword;
    QString encryption = QStringLiteral("none");
    QString flow;
    QString remark;
    bool enabled = true;

    QString network = QStringLiteral("tcp");
    QString path;
    QString host;
    QString headerType;

    QString security;
    QString serverName;
    QString sni;
    QString fingerprint;
    QString publicKey;
    QString shortId;
    QString spiderX;
    bool allowInsecure = false;

    int alterId = 0;

    ProfileSourceType sourceType = ProfileSourceType::Manual;
    QString subscriptionId;
    QString subscriptionName;
    QString sourceKey;
    QDateTime lastSeenAt;
    bool deletedBySubscriptionUpdate = false;

    static Profile createDefault();
    static Profile createVlessRealityDefault();

    bool isValid() const;
    bool isSecurityReality() const;
    bool isSecurityTls() const;
    QString effectiveServerName() const;
    QString effectiveEncryption() const;
    QString effectiveFingerprint() const;

    QString computeSourceKey() const;
    bool isManual() const;
    bool isFromSubscription(const QString& subscriptionId) const;
};

} // namespace zarya
