#pragma once

#include <guiddef.h>

namespace zarya {

extern const GUID ZARYA_WFP_PROVIDER_KEY;
extern const GUID ZARYA_WFP_SUBLAYER_KEY;

extern const GUID ZARYA_FILTER_ALLOW_LOOPBACK_V4;
extern const GUID ZARYA_FILTER_ALLOW_LOOPBACK_V6;
extern const GUID ZARYA_FILTER_ALLOW_TUN_IFACE_V4;
extern const GUID ZARYA_FILTER_ALLOW_TUN_IFACE_V6;
extern const GUID ZARYA_FILTER_BLOCK_OUTBOUND_V4;
extern const GUID ZARYA_FILTER_BLOCK_OUTBOUND_V6;

const GUID* zaryaProxyAllowFilterV4Key(int index);
const GUID* zaryaProxyAllowFilterV6Key(int index);
int zaryaMaxProxyAllowFilters();

void zaryaAllZaryaFilterKeys(GUID* keys, int* count);

} // namespace zarya
