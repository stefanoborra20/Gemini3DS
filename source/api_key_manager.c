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

static void MaskKey(const char *key, char *out, size_t n) {
    size_t len = key ? strlen(key) : 0;

    if (len == 0) {
        snprintf(out, n, "NONE");
        return;
    }

    if (len <= 8) {
        snprintf(out, n, "****");
        return;
    }

    snprintf(out, n, "%.4s****%s", key, key + len - 4);
}

void ApiManager_Init() {
    CountKeysInFile();
    current_index = 0;
    printApiKeyErr = (keys_num == 0);

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

    printApiKeyErr = (keys_num == 0);
}

static void SwitchKey(int delta) {
    if (keys_num <= 1) return;

    int index = current_index + delta;
    if (index < 0) index = keys_num - 1;
    if (index >= keys_num) index = 0;

    ApiManager_SetActiveKeyIndex(index);
}

static void EditCurrentKey(void) {
    char tempBuffer[API_KEY_MAX_LEN];

    if (keys_num > 0) {
        snprintf(tempBuffer, sizeof(tempBuffer), "%s", active_key);
    } else {
        tempBuffer[0] = '\0';
    }

    if (R_OpenKeyboard("Edit key (clear text to delete)", tempBuffer, sizeof(tempBuffer))) {
        ApiManager_UpdateCurrentKey(tempBuffer);
    }
}

static void AddNewKey(void) {
    char tempBuffer[API_KEY_MAX_LEN] = "";

    if (R_OpenKeyboard("Add new Gemini API key", tempBuffer, sizeof(tempBuffer))) {
        if (strlen(tempBuffer) > 0) {
            ApiManager_AddNewKey(tempBuffer);
            printApiKeyErr = (ApiManager_GetTotalKeys() == 0);
        }
    }
}

bool ApiManager_Update(u32 kDown) {
    touchPosition touch;
    bool touchDown = false;
    bool touchUp = false;
    R_UpdateTouch(&touch, &touchDown, &touchUp);

    if (touchDown) {
        if (R_TouchInRect(&touch, 8.0f, 34.0f, 148.0f, 44.0f)) {
            SwitchKey(-1);
            return false;
        }
        if (R_TouchInRect(&touch, 164.0f, 34.0f, 148.0f, 44.0f)) {
            SwitchKey(1);
            return false;
        }
        if (R_TouchInRect(&touch, 8.0f, 86.0f, 148.0f, 44.0f)) {
            ApiManager_ToggleRotation(!ApiManager_IsRotationEnabled());
            return false;
        }
        if (R_TouchInRect(&touch, 164.0f, 86.0f, 148.0f, 44.0f)) {
            EditCurrentKey();
            return false;
        }
        if (R_TouchInRect(&touch, 8.0f, 138.0f, 148.0f, 44.0f)) {
            AddNewKey();
            return false;
        }
        if (R_TouchInRect(&touch, 164.0f, 138.0f, 148.0f, 44.0f)) {
            return true;
        }
    }

    if (kDown & KEY_L) SwitchKey(-1);
    if (kDown & KEY_R) SwitchKey(1);
    if (kDown & KEY_Y) ApiManager_ToggleRotation(!ApiManager_IsRotationEnabled());
    if (kDown & KEY_A) EditCurrentKey();
    if (kDown & KEY_X) AddNewKey();
    if (kDown & KEY_B) return true;

    return false;
}

void ApiManager_Draw() {
    char masked[48];
    char page[32];

    /* ------------------------------------------------------------------ */
    /* Top screen                                                         */
    /* ------------------------------------------------------------------ */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    if (keys_num > 0) {
        MaskKey(active_key, masked, sizeof(masked));
        R_DrawTextCentered(0.0f, 104.0f, SCREEN_TOP_WIDTH, 0.85f,
                           masked, COLOR_TEXT_HIGHLIGHT);

        snprintf(page, sizeof(page), "%d/%d", current_index + 1, keys_num);
        R_DrawTextRight(390.0f, 16.0f, 0.45f, page, COLOR_TEXT_DIM);
    } else {
        R_DrawTextCentered(0.0f, 104.0f, SCREEN_TOP_WIDTH, 0.85f,
                           "NO KEY", COLOR_ALERT);
    }

    /* ------------------------------------------------------------------ */
    /* Bottom screen                                                      */
    /* ------------------------------------------------------------------ */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    R_DrawButtonLeft(8.0f, 34.0f, 148.0f, 44.0f, "L", "PREV", BTN_NORMAL);
    R_DrawButtonLeft(164.0f, 34.0f, 148.0f, 44.0f, "R", "NEXT", BTN_NORMAL);

    R_DrawButtonLeft(8.0f, 86.0f, 148.0f, 44.0f, "Y", "ROTATE",
                     auto_rotate_enabled ? BTN_SELECTED : BTN_NORMAL);
    R_DrawButtonLeft(164.0f, 86.0f, 148.0f, 44.0f, "A", "EDIT", BTN_NORMAL);

    R_DrawButtonLeft(8.0f, 138.0f, 148.0f, 44.0f, "X", "ADD",
                     keys_num == 0 ? BTN_ACTIVE : BTN_NORMAL);
    R_DrawButtonLeft(164.0f, 138.0f, 148.0f, 44.0f, "B", "BACK", BTN_NORMAL);
}
