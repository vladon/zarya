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
void ZaryaXrayFree(void* value);

#ifdef __cplusplus
}
#endif
