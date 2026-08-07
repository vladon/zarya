#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ZaryaSingBoxRuntimeState {
    ZARYA_SINGBOX_STOPPED = 0,
    ZARYA_SINGBOX_STARTING = 1,
    ZARYA_SINGBOX_RUNNING = 2,
    ZARYA_SINGBOX_STOPPING = 3,
    ZARYA_SINGBOX_FAILED = 4
};

int ZaryaSingBoxAbiVersion(void);
char* ZaryaSingBoxVersion(void);
char* ZaryaSingBoxValidate(const char* config_json, size_t config_size);
char* ZaryaSingBoxStart(const char* config_json, size_t config_size);
char* ZaryaSingBoxStop(void);
int ZaryaSingBoxState(void);
char* ZaryaSingBoxDrainLogs(void);
void ZaryaSingBoxFree(void* value);

#ifdef __cplusplus
}
#endif
