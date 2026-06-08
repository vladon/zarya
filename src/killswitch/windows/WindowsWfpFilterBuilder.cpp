#include "killswitch/windows/WindowsWfpPreamble.h"

#include "killswitch/windows/WindowsWfpFilterBuilder.h"

#include "killswitch/KillSwitchLog.h"
#include "killswitch/windows/WindowsWfpGuids.h"
#include "killswitch/windows/WindowsWfpSession.h"
#include "killswitch/windows/WindowsWfpUtils.h"

namespace zarya {

namespace {

constexpr UINT16 kAllowWeight = 1000;
constexpr UINT16 kBlockWeight = 100;

bool matchesTunHint(const QString& text, const QString& preferredName)
{
    const QString lower = text.toLower();
    if (!preferredName.isEmpty() && lower.contains(preferredName.toLower())) {
        return true;
    }
    return lower.contains(QStringLiteral("zarya-tun")) || lower.contains(QStringLiteral("sing-box"))
           || lower.contains(QStringLiteral("wintun"));
}

} // namespace

bool WindowsWfpFilterBuilder::findTunInterface(const QString& preferredName,
                                               WindowsWfpTunInterfaceInfo* info,
                                               QString* warningMessage)
{
    if (!info) {
        return false;
    }
    *info = {};

    ULONG bufferSize = 0;
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize)
        != ERROR_BUFFER_OVERFLOW) {
        if (warningMessage) {
            *warningMessage = QStringLiteral("Could not enumerate network adapters.");
        }
        return false;
    }

    QByteArray buffer(static_cast<int>(bufferSize), Qt::Uninitialized);
    auto* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    const ULONG result =
        GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &bufferSize);
    if (result != NO_ERROR) {
        if (warningMessage) {
            *warningMessage = QStringLiteral("GetAdaptersAddresses failed (0x%1).")
                                  .arg(result, 0, 16);
        }
        return false;
    }

    for (auto* adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
        const QString friendlyName = QString::fromWCharArray(adapter->FriendlyName);
        const QString description = QString::fromWCharArray(adapter->Description);
        if (!matchesTunHint(friendlyName, preferredName)
            && !matchesTunHint(description, preferredName)) {
            continue;
        }

        info->found = true;
        info->friendlyName = friendlyName;
        info->luid = adapter->Luid;
        info->ipv4IfIndex = adapter->IfIndex;
        info->ipv6IfIndex = adapter->Ipv6IfIndex;
        return true;
    }

    if (warningMessage) {
        *warningMessage =
            QStringLiteral("Could not add TUN interface-specific allow filter; PoC may block too "
                           "aggressively.");
    }
    return false;
}

bool WindowsWfpFilterBuilder::addFilter(WindowsWfpSession& session, FWPM_FILTER0* filter,
                                        QString* errorMessage)
{
    const DWORD result = FwpmFilterAdd0(session.engine(), filter, nullptr, nullptr);
    if (result != ERROR_SUCCESS) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }
    return true;
}

bool WindowsWfpFilterBuilder::addLoopbackAllows(WindowsWfpSession& session, bool allowLoopback,
                                                QString* errorMessage)
{
    if (!allowLoopback) {
        return true;
    }

    killSwitchLog(QStringLiteral("Adding loopback allow filters"));

    {
        FWPM_FILTER_CONDITION0 conditions[1] = {};
        FWP_V4_ADDR_AND_MASK v4Address = {};
        v4Address.addr = htonl(0x7F000000U);
        v4Address.mask = htonl(0xFF000000U);
        conditions[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
        conditions[0].matchType = FWP_MATCH_EQUAL;
        conditions[0].conditionValue.type = FWP_V4_ADDR_MASK;
        conditions[0].conditionValue.v4AddrMask = &v4Address;

        FWPM_FILTER0 filter = {};
        filter.filterKey = ZARYA_FILTER_ALLOW_LOOPBACK_V4;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Allow Loopback IPv4");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kAllowWeight;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.numFilterConditions = 1;
        filter.filterCondition = conditions;
        if (!addFilter(session, &filter, errorMessage)) {
            return false;
        }
    }

    {
        FWPM_FILTER_CONDITION0 conditions[1] = {};
        FWP_BYTE_ARRAY16 v6Address = {};
        v6Address.byteArray16[15] = 0x01;
        conditions[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
        conditions[0].matchType = FWP_MATCH_EQUAL;
        conditions[0].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;
        conditions[0].conditionValue.byteArray16 = &v6Address;

        FWPM_FILTER0 filter = {};
        filter.filterKey = ZARYA_FILTER_ALLOW_LOOPBACK_V6;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Allow Loopback IPv6");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kAllowWeight;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.numFilterConditions = 1;
        filter.filterCondition = conditions;
        if (!addFilter(session, &filter, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool WindowsWfpFilterBuilder::addProxyAllows(WindowsWfpSession& session,
                                             const KillSwitchRuleSet& rules, QStringList* warnings,
                                             QString* errorMessage)
{
    if (rules.proxyServerIpv4.isEmpty() && rules.proxyServerIpv6.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Cannot enable Windows WFP kill switch because proxy server IPs are not resolved.");
        }
        return false;
    }

    int v4Index = 0;
    for (const QString& ip : rules.proxyServerIpv4) {
        if (v4Index >= zaryaMaxProxyAllowFilters()) {
            if (warnings) {
                warnings->append(QStringLiteral("Too many IPv4 proxy allow filters; extras skipped."));
            }
            break;
        }
        const GUID* filterKey = zaryaProxyAllowFilterV4Key(v4Index);
        if (!filterKey) {
            break;
        }

        UINT32 hostOrder = 0;
        if (!wfpParseIpv4(ip, &hostOrder)) {
            if (warnings) {
                warnings->append(QStringLiteral("Skipped invalid IPv4 proxy address: %1").arg(ip));
            }
            continue;
        }

        FWPM_FILTER_CONDITION0 conditions[2] = {};
        FWP_V4_ADDR_AND_MASK v4Address = {};
        v4Address.addr = htonl(hostOrder);
        v4Address.mask = 0xFFFFFFFFU;
        conditions[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
        conditions[0].matchType = FWP_MATCH_EQUAL;
        conditions[0].conditionValue.type = FWP_V4_ADDR_MASK;
        conditions[0].conditionValue.v4AddrMask = &v4Address;

        const UINT16 port = static_cast<UINT16>(rules.proxyServerPort);
        conditions[1].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
        conditions[1].matchType = FWP_MATCH_EQUAL;
        conditions[1].conditionValue.type = FWP_UINT16;
        conditions[1].conditionValue.uint16 = port;

        FWPM_FILTER0 filter = {};
        filter.filterKey = *filterKey;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Allow Proxy IPv4");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kAllowWeight;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.numFilterConditions = 2;
        filter.filterCondition = conditions;

        killSwitchLog(QStringLiteral("Adding proxy allow filter: %1:%2").arg(ip).arg(port));
        if (!addFilter(session, &filter, errorMessage)) {
            return false;
        }
        ++v4Index;
    }

    int v6Index = 0;
    for (const QString& ip : rules.proxyServerIpv6) {
        if (v6Index >= zaryaMaxProxyAllowFilters()) {
            if (warnings) {
                warnings->append(QStringLiteral("Too many IPv6 proxy allow filters; extras skipped."));
            }
            break;
        }
        const GUID* filterKey = zaryaProxyAllowFilterV6Key(v6Index);
        if (!filterKey) {
            break;
        }

        UINT8 address[16] = {};
        if (!wfpParseIpv6(ip, address)) {
            if (warnings) {
                warnings->append(QStringLiteral("Skipped invalid IPv6 proxy address: %1").arg(ip));
            }
            continue;
        }

        FWPM_FILTER_CONDITION0 conditions[2] = {};
        FWP_BYTE_ARRAY16 v6Address = {};
        memcpy(v6Address.byteArray16, address, sizeof(address));
        conditions[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
        conditions[0].matchType = FWP_MATCH_EQUAL;
        conditions[0].conditionValue.type = FWP_BYTE_ARRAY16_TYPE;
        conditions[0].conditionValue.byteArray16 = &v6Address;

        const UINT16 port = static_cast<UINT16>(rules.proxyServerPort);
        conditions[1].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
        conditions[1].matchType = FWP_MATCH_EQUAL;
        conditions[1].conditionValue.type = FWP_UINT16;
        conditions[1].conditionValue.uint16 = port;

        FWPM_FILTER0 filter = {};
        filter.filterKey = *filterKey;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Allow Proxy IPv6");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kAllowWeight;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.numFilterConditions = 2;
        filter.filterCondition = conditions;

        killSwitchLog(QStringLiteral("Adding proxy allow filter: [%1]:%2").arg(ip).arg(port));
        if (!addFilter(session, &filter, errorMessage)) {
            return false;
        }
        ++v6Index;
    }

    if (v4Index == 0 && v6Index == 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Cannot enable Windows WFP kill switch because proxy server IPs are not resolved.");
        }
        return false;
    }

    return true;
}

bool WindowsWfpFilterBuilder::addTunInterfaceAllows(WindowsWfpSession& session,
                                                    const WindowsWfpTunInterfaceInfo& tunInfo,
                                                    QString* warningMessage, QString* errorMessage)
{
    if (!tunInfo.found) {
        if (warningMessage) {
            killSwitchLog(*warningMessage);
        }
        return true;
    }

    UINT64 luidValue = tunInfo.luid.Value;

    {
        FWPM_FILTER_CONDITION0 conditions[1] = {};
        conditions[0].fieldKey = FWPM_CONDITION_IP_LOCAL_INTERFACE;
        conditions[0].matchType = FWP_MATCH_EQUAL;
        conditions[0].conditionValue.type = FWP_UINT64;
        conditions[0].conditionValue.uint64 = &luidValue;

        FWPM_FILTER0 filter = {};
        filter.filterKey = ZARYA_FILTER_ALLOW_TUN_IFACE_V4;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Allow TUN Interface IPv4");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kAllowWeight;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.numFilterConditions = 1;
        filter.filterCondition = conditions;

        killSwitchLog(QStringLiteral("Adding TUN interface allow filter: %1 (LUID %2)")
                          .arg(tunInfo.friendlyName)
                          .arg(luidValue));
        if (!addFilter(session, &filter, errorMessage)) {
            if (warningMessage) {
                *warningMessage =
                    QStringLiteral("Could not add TUN interface-specific allow filter; PoC may "
                                   "block too aggressively.");
            }
            killSwitchLog(*warningMessage);
            return true;
        }
    }

    {
        FWPM_FILTER_CONDITION0 conditions[1] = {};
        conditions[0].fieldKey = FWPM_CONDITION_IP_LOCAL_INTERFACE;
        conditions[0].matchType = FWP_MATCH_EQUAL;
        conditions[0].conditionValue.type = FWP_UINT64;
        conditions[0].conditionValue.uint64 = &luidValue;

        FWPM_FILTER0 filter = {};
        filter.filterKey = ZARYA_FILTER_ALLOW_TUN_IFACE_V6;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Allow TUN Interface IPv6");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kAllowWeight;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.numFilterConditions = 1;
        filter.filterCondition = conditions;

        if (!addFilter(session, &filter, errorMessage)) {
            if (warningMessage) {
                *warningMessage =
                    QStringLiteral("Could not add TUN interface-specific allow filter; PoC may "
                                   "block too aggressively.");
            }
            killSwitchLog(*warningMessage);
            return true;
        }
    }

    return true;
}

bool WindowsWfpFilterBuilder::addOutboundBlocks(WindowsWfpSession& session, QString* errorMessage)
{
    killSwitchLog(QStringLiteral("Adding outbound block filters"));

    {
        FWPM_FILTER0 filter = {};
        filter.filterKey = ZARYA_FILTER_BLOCK_OUTBOUND_V4;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Block Outbound IPv4");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kBlockWeight;
        filter.action.type = FWP_ACTION_BLOCK;
        filter.numFilterConditions = 0;
        if (!addFilter(session, &filter, errorMessage)) {
            return false;
        }
    }

    {
        FWPM_FILTER0 filter = {};
        filter.filterKey = ZARYA_FILTER_BLOCK_OUTBOUND_V6;
        filter.displayData.name = const_cast<wchar_t*>(L"Zarya Block Outbound IPv6");
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
        filter.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
        filter.weight.type = FWP_UINT16;
        filter.weight.uint16 = kBlockWeight;
        filter.action.type = FWP_ACTION_BLOCK;
        filter.numFilterConditions = 0;
        if (!addFilter(session, &filter, errorMessage)) {
            return false;
        }
    }

    return true;
}

QStringList WindowsWfpFilterBuilder::activeRuleDescriptions()
{
    return {QStringLiteral("Zarya WFP provider"),
            QStringLiteral("Zarya WFP sublayer"),
            QStringLiteral("Zarya WFP filters (ALE_AUTH_CONNECT)")};
}

} // namespace zarya
