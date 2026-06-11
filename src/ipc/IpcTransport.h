#pragma once

#include <QString>

namespace zarya {

class IpcTransport {
public:
    static QString defaultServerName();
    static QString serviceServerName(const QString& serviceName);
};

} // namespace zarya
