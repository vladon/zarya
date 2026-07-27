#pragma once

#include "domain/CoreType.h"
#include "domain/ProfileSourceType.h"
#include "domain/ProtocolType.h"
#include "testing/TestStatus.h"

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
    QString password;
    QString encryption = QStringLiteral("none");
    QString method;
    QString securityCipher;
    QString flow;
    QString remark;
    QString alpn;
    QString obfs;
    QString obfsPassword;
    bool enabled = true;

    QString network = QStringLiteral("tcp");
    QString path;
    QString host;
    QString headerType;
    QString serviceName;

    QString security;
    QString serverName;
    QString sni;
    QString fingerprint;
    QString publicKey;
    QString shortId;
    QString spiderX;
    bool allowInsecure = false;

    // WireGuard: password holds private key; publicKey is peer public key.
    QString localAddress;
    QString allowedIps;
    QString preSharedKey;
    QString reserved;
    int mtu = 0;
    int keepAlive = 0;

    int alterId = 0;

    ProfileSourceType sourceType = ProfileSourceType::Manual;
    QString subscriptionId;
    QString subscriptionName;
    QString sourceKey;
    QDateTime lastSeenAt;
    bool deletedBySubscriptionUpdate = false;
    QString unsupportedReason;

    int lastTcpPingMs = -1;
    int lastRealDelayMs = -1;
    TestStatus lastTestStatus = TestStatus::NeverTested;
    QString lastTestError;
    QDateTime lastTestedAt;

    static Profile createDefault();
    static Profile createVlessRealityDefault();

    bool isValid() const;
    bool isSecurityReality() const;
    bool isSecurityTls() const;
    QString effectiveServerName() const;
    QString effectiveEncryption() const;
    QString effectiveFingerprint() const;
    QString effectiveUuid() const;
    QString effectivePassword() const;
    QString effectiveVmessSecurity() const;
    QString effectiveMethod() const;
    bool isSecurityNone() const;
    bool hasUnsupportedFeature() const;

    QString computeSourceKey() const;
    bool isManual() const;
    bool isFromSubscription(const QString& subscriptionId) const;
};

} // namespace zarya
