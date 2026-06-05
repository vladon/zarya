#pragma once

#include "domain/DnsProfile.h"
#include "runtime/singbox/SingBoxRuleSetContext.h"

#include <QJsonObject>
#include <QSet>
#include <QStringList>

namespace zarya {

struct SingBoxDnsGenerationOptions {
    bool tunMode = true;
    bool enableDnsHijack = true;
    QString finalDnsServerTag = QStringLiteral("remote-1");
    SingBoxRuleSetContext ruleSetContext;
    QSet<QString>* usedRuleSetTagsOut = nullptr;
};

class SingBoxDnsGenerator {
public:
    QJsonObject generateDns(const DnsProfile& dnsProfile, const SingBoxDnsGenerationOptions& options,
                            QStringList* warnings = nullptr) const;
};

} // namespace zarya
