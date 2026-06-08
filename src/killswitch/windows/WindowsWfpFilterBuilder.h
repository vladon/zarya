#pragma once

#include "killswitch/KillSwitchRuleSet.h"

#include <QString>
#include <QStringList>

#include "killswitch/windows/WindowsWfpPreamble.h"

namespace zarya {

class WindowsWfpSession;

struct WindowsWfpTunInterfaceInfo {
    bool found = false;
    NET_LUID luid = {};
    ULONG ipv4IfIndex = 0;
    ULONG ipv6IfIndex = 0;
    QString friendlyName;
};

class WindowsWfpFilterBuilder {
public:
    static bool findTunInterface(const QString& preferredName, WindowsWfpTunInterfaceInfo* info,
                                 QString* warningMessage);

    static bool addLoopbackAllows(WindowsWfpSession& session, bool allowLoopback,
                                  QString* errorMessage);
    static bool addProxyAllows(WindowsWfpSession& session, const KillSwitchRuleSet& rules,
                               QStringList* warnings, QString* errorMessage);
    static bool addTunInterfaceAllows(WindowsWfpSession& session,
                                      const WindowsWfpTunInterfaceInfo& tunInfo,
                                      QString* warningMessage, QString* errorMessage);
    static bool addOutboundBlocks(WindowsWfpSession& session, QString* errorMessage);

    static QStringList activeRuleDescriptions();

private:
    static bool addFilter(WindowsWfpSession& session, FWPM_FILTER0* filter, QString* errorMessage);
};

} // namespace zarya
