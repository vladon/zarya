#pragma once

#include "domain/DnsProfile.h"

#include <QJsonObject>
#include <QStringList>

namespace zarya {

struct SingBoxDnsGenerationOptions {
    bool tunMode = true;
    bool enableDnsHijack = true;
    QString finalDnsServerTag = QStringLiteral("remote-1");
};

class SingBoxDnsGenerator {
public:
    QJsonObject generateDns(const DnsProfile& dnsProfile, const SingBoxDnsGenerationOptions& options,
                            QStringList* warnings = nullptr) const;
};

} // namespace zarya
