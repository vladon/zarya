#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ZaryaXrayRuntimeState {
    ZARYA_XRAY_STOPPED = 0,
    ZARYA_XRAY_STARTING = 1,
    ZARYA_XRAY_RUNNING = 2,
    ZARYA_XRAY_STOPPING = 3,
    ZARYA_XRAY_FAILED = 4
};

int ZaryaXrayAbiVersion(void);
char* ZaryaXrayVersion(void);
char* ZaryaXrayValidate(const char* config_json, size_t config_size, const char* asset_dir);
char* ZaryaXrayStart(const char* config_json, size_t config_size, const char* asset_dir);
char* ZaryaXrayStop(void);
int ZaryaXrayState(void);
char* ZaryaXrayDrainLogs(void);
char* ZaryaXrayProbeURL(const char* target_url, const char* proxy_kind,
    const char* proxy_host, int proxy_port, int timeout_ms,
    long long* delay_ms);
void ZaryaXrayFree(void* value);

#ifdef __cplusplus
}
#endif
