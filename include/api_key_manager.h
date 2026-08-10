#ifndef API_KEY_MANAGER_H
#define API_KEY_MANAGER_H

#include <3ds.h>
#include <stdbool.h>

#define KEY_FILE_PATH "gemini_api_key.txt"
#define API_KEY_MAX_LEN 64

void ApiManager_Init();
const char* ApiManager_GetActiveKey();
void ApiManager_RotateKey();
void ApiManager_ToggleRotation(bool enable);
bool ApiManager_IsRotationEnabled();

void ApiManager_AddNewKey(const char*newKey);
int ApiManager_GetTotalKeys();
int ApiManager_GetActiveKeyIndex();
void ApiManager_SetActiveKeyIndex(int index);
void ApiManager_UpdateCurrentKey(const char *newKey);

void ApiManager_Update(u32 kDown);
void ApiManager_Draw();

#endif
