#ifndef API_KEY_MANAGER_H
#define API_KEY_MANAGER_H

#include <3ds.h>
#include <stdbool.h>

#define API_KEY_MAX_LEN 64

void ApiManager_Init(const char* filepath);

const char* ApiManager_GetActiveKey(void);

void ApiManager_RotateKey(void);

void ApiManager_ToggleRotation(bool enable);
bool ApiManager_IsRotationEnabled(void);

#endif 
