#include "api_key_manager.h"
#include <stdio.h>
#include <string.h>

static char active_key[API_KEY_MAX_LEN] = "";
static char next_key[API_KEY_MAX_LEN] = "";
static char key_file_path[256] = "";

static int current_index = 0;
static int keys_num = 0;
static bool auto_rotate_enabled = false;

static void ReadLineFromFile(int targetLine, char* outBuffer) {
    FILE *f = fopen(key_file_path, "r");
    if (!f) {
        outBuffer[0] = '\0';
        return;
    }

    char lineBuffer[128];
    int validLineCount = 0;

    while (fgets(lineBuffer, sizeof(lineBuffer), f)) {
        // Strip trailing newlines / carriage returns
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

static void CountKeysInFile(void) {
    FILE *f = fopen(key_file_path, "r");
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

void ApiManager_Init(const char* filepath) {
    if (filepath && strlen(filepath) > 0) {
        strncpy(key_file_path, filepath, sizeof(key_file_path) - 1);
        key_file_path[sizeof(key_file_path) - 1] = '\0';
    }

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

const char* ApiManager_GetActiveKey(void) {
    return active_key;
}

void ApiManager_RotateKey(void) {
    if (keys_num <= 1) return; // Nothing to rotate to

    strncpy(active_key, next_key, API_KEY_MAX_LEN);

    current_index = (current_index + 1) % keys_num;

    int next_idx = (current_index + 1) % keys_num;
    ReadLineFromFile(next_idx, next_key);
}

void ApiManager_ToggleRotation(bool enable) {
    auto_rotate_enabled = enable;
}

bool ApiManager_IsRotationEnabled(void) {
    return auto_rotate_enabled;
}
