#include "api_key_manager.h"
#include "renderer.h"
#include <stdio.h>
#include <string.h>

extern bool printApiKeyErr;

static char active_key[API_KEY_MAX_LEN] = "";
static char next_key[API_KEY_MAX_LEN] = "";

static int current_index = 0;
static int keys_num = 0;
static bool auto_rotate_enabled = false;

static void ReadLineFromFile(int targetLine, char* outBuffer) {
    FILE *f = fopen(KEY_FILE_PATH, "r");
    if (!f) {
        outBuffer[0] = '\0';
        return;
    }

    char lineBuffer[128];
    int validLineCount = 0;

    while (fgets(lineBuffer, sizeof(lineBuffer), f)) {
        lineBuffer[strcspn(lineBuffer, "\r\n")] = 0;

        if (strlen(lineBuffer) > 0) {
            if (validLineCount == targetLine) {
                strncpy(outBuffer, lineBuffer, API_KEY_MAX_LEN - 1);
                outBuffer[API_KEY_MAX_LEN - 1] = '\0';
                fclose(f);
                return;
            }
            validLineCount++;
        }
    }

    fclose(f);
    outBuffer[0] = '\0';
}

static void CountKeysInFile() {
    FILE *f = fopen(KEY_FILE_PATH, "r");
    keys_num = 0;
    if (!f) return;

    char lineBuffer[128];
    while (fgets(lineBuffer, sizeof(lineBuffer), f)) {
        lineBuffer[strcspn(lineBuffer, "\r\n")] = 0;
        if (strlen(lineBuffer) > 0) {
            keys_num++;
        }
    }
    fclose(f);
}

void ApiManager_Init() {
    CountKeysInFile();
    current_index = 0;

    if (keys_num > 0) {
        ReadLineFromFile(0, active_key);
        int next_idx = (keys_num > 1) ? 1 : 0;
        ReadLineFromFile(next_idx, next_key);
    } else {
        active_key[0] = '\0';
        next_key[0] = '\0';
    }
}

const char* ApiManager_GetActiveKey() {
    return active_key;
}

void ApiManager_RotateKey() {
    if (keys_num <= 1) return;

    strncpy(active_key, next_key, API_KEY_MAX_LEN);
    current_index = (current_index + 1) % keys_num;

    int next_idx = (current_index + 1) % keys_num;
    ReadLineFromFile(next_idx, next_key);
}

void ApiManager_ToggleRotation(bool enable) {
    auto_rotate_enabled = enable;
}

bool ApiManager_IsRotationEnabled() {
    return auto_rotate_enabled;
}

int ApiManager_GetTotalKeys() {
    return keys_num;
}

int ApiManager_GetActiveKeyIndex() {
    return current_index;
}

void ApiManager_SetActiveKeyIndex(int index) {
    if (keys_num <= 0) return;

    current_index = index;
    if (current_index < 0) current_index = keys_num - 1;
    if (current_index >= keys_num) current_index = 0;

    ReadLineFromFile(current_index, active_key);
    int next_idx = (keys_num > 1) ? (current_index + 1) % keys_num : 0;
    ReadLineFromFile(next_idx, next_key);
}

void ApiManager_AddNewKey(const char* newKey) {
    FILE *f = fopen(KEY_FILE_PATH, "a");
    if (!f) f = fopen(KEY_FILE_PATH, "w"); 
    
    if (f) {
        fprintf(f, "%s\n", newKey);
        fclose(f);
    }
    
    CountKeysInFile();
    ApiManager_SetActiveKeyIndex(keys_num - 1);
}

void ApiManager_UpdateCurrentKey(const char* newKey) {
    FILE *f = fopen(KEY_FILE_PATH, "r");
    FILE *temp = fopen("temp_keys.txt", "w");
    
    if (!temp) {
        if (f) fclose(f);
        return;
    }
    
    bool is_deleting = (strlen(newKey) == 0);
    
    if (f) {
        char lineBuffer[128];
        int currentLine = 0;
        while (fgets(lineBuffer, sizeof(lineBuffer), f)) {
            lineBuffer[strcspn(lineBuffer, "\r\n")] = 0;
            if (strlen(lineBuffer) > 0) {
                if (currentLine == current_index) {
                    if (!is_deleting) {
                        fprintf(temp, "%s\n", newKey);
                    }
                } else {
                    fprintf(temp, "%s\n", lineBuffer); 
                }
                currentLine++;
            }
        }
        fclose(f);
    } else {
        if (!is_deleting) fprintf(temp, "%s\n", newKey);
    }
    
    fclose(temp);
    
    remove(KEY_FILE_PATH);
    rename("temp_keys.txt", KEY_FILE_PATH);
    
    CountKeysInFile();
    
    if (keys_num == 0) {
        active_key[0] = '\0';
        next_key[0] = '\0';
        current_index = 0;
    } else {
        if (current_index >= keys_num) current_index = keys_num - 1;
        ApiManager_SetActiveKeyIndex(current_index);
    }
}

void ApiManager_Update(u32 kDown) {
    if (kDown & KEY_L) ApiManager_SetActiveKeyIndex(ApiManager_GetActiveKeyIndex() - 1);
    if (kDown & KEY_R) ApiManager_SetActiveKeyIndex(ApiManager_GetActiveKeyIndex() + 1);
    
    if (kDown & KEY_Y) ApiManager_ToggleRotation(!ApiManager_IsRotationEnabled());

    // EDIT CURRENT KEY
    if (kDown & KEY_A) {
        char tempBuffer[API_KEY_MAX_LEN];
        snprintf(tempBuffer, API_KEY_MAX_LEN, "%s", active_key);
        if (strcmp(tempBuffer, "No Api Key") == 0) tempBuffer[0] = '\0'; 

        if (R_OpenKeyboard("Edit Key (Clear text to delete)", tempBuffer, API_KEY_MAX_LEN)) {
            ApiManager_UpdateCurrentKey(tempBuffer);
            printApiKeyErr = false;
        }
    } 
    
    // ADD NEW KEY
    if (kDown & KEY_X) {
        char tempBuffer[API_KEY_MAX_LEN] = "";
        
        if (R_OpenKeyboard("Add New Gemini API Key", tempBuffer, API_KEY_MAX_LEN)) {
            if (strlen(tempBuffer) > 0) {
                ApiManager_AddNewKey(tempBuffer);
                printApiKeyErr = false;
            }
        }
    }
}

void ApiManager_Draw() {
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
    
    char headerMsg[64];
    snprintf(headerMsg, 64, "Current Key: %d of %d", keys_num > 0 ? current_index + 1 : 0, keys_num);
    R_DrawText(10, 10, 1.0f, headerMsg, COLOR_TEXT_NORMAL);

    if (keys_num == 0) {
        R_DrawText(10, 40, 0.5f, "No Api Keys saved. Press [X] to add one.", COLOR_TEXT_HIGHLIGHT);
    } else {
        int keyLen = strlen(active_key);
        if (keyLen > 40) {
            char line1[45];
            char line2[64];
            snprintf(line1, sizeof(line1), "%.40s", active_key); 
            snprintf(line2, sizeof(line2), "%s", active_key + 40); 
            
            R_DrawText(10, 40, 0.5f, line1, COLOR_TEXT_HIGHLIGHT);
            R_DrawText(10, 55, 0.5f, line2, COLOR_TEXT_HIGHLIGHT);
        } else {
            R_DrawText(10, 40, 0.5f, active_key, COLOR_TEXT_HIGHLIGHT);
        }
    }
    
    char rotMsg[64];
    snprintf(rotMsg, 64, "Auto-Rotate: %s", auto_rotate_enabled ? "ON" : "OFF");
    R_DrawText(10, 85, 0.8f, rotMsg, auto_rotate_enabled ? COLOR_TEXT_HIGHLIGHT : COLOR_TEXT_NORMAL);
    
    R_DrawText(10, 115, 0.8f, "[L/R] Switch Key", COLOR_TEXT_NORMAL);
    R_DrawText(10, 135, 0.8f, "[Y] Toggle Auto-Rotate", COLOR_TEXT_NORMAL);
    R_DrawText(10, 155, 0.8f, "[A] Edit/Delete Key", COLOR_TEXT_NORMAL); // Updated Label
    R_DrawText(10, 175, 0.8f, "[X] Add New Key", COLOR_TEXT_NORMAL);
    R_DrawText(10, 195, 0.8f, "[B] Back", COLOR_TEXT_NORMAL);
}
