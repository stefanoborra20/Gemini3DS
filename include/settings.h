#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "renderer.h"

typedef struct SettingOption SettingOption;
typedef void (*toStringFunc)(SettingOption *self, char *buffer, size_t size);
typedef void (*EditFunc)(SettingOption *self, int delta);

typedef enum {
    MODEL_2_5_FLASH=0,
    MODEL_2_5_FLASH_LITE,
    MODEL_COUNT
} GeminiModel;

struct SettingOption {
    const char *label;

    union {
        int iVal;
        float fVal; 
        char bVal; 
    } value;

    struct {
        float min;
        float max;
        float step; // How much it changes per click
    } config;

    toStringFunc toString;
    EditFunc onEdit;
}; 

void Settings_Init();

void Settings_Update(u32 kDown);

void Settings_Draw();

// Public getters
const char* Settings_GetModel();
float Settings_GetTemperature();
int Settings_GetMaxTokens();

#endif
