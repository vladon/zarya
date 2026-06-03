#pragma once

#include "domain/CoreType.h"
#include "domain/ProtocolType.h"

#include <QString>

namespace zarya {

struct Profile {
    QString id;
    QString name;
    ProtocolType protocol = ProtocolType::Vless;
    QString address;
    int port = 443;
    QString uuidPassword;
    QString security;
    QString network;
    QString sni;
    QString flow;
    QString remark;
    bool enabled = true;
    CoreType coreType = CoreType::Xray;

    static Profile createDefault();
    bool isValid() const;
};

} // namespace zarya
