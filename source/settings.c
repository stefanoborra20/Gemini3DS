#include "settings.h"

void IntToString(SettingOption*, char*, size_t);
void FloatToString(SettingOption*, char*, size_t);
void BoolToString(SettingOption*, char*, size_t);
void FloatToString(SettingOption*, char*, size_t);
void ModelToString(SettingOption*, char*, size_t);

void EditInt(SettingOption*, int);
void EditFloat(SettingOption*, int);
void EditBool(SettingOption*, int);
void EditCycle(SettingOption*, int);
void EditInt(SettingOption*, int);

enum {OPT_MODEL=0, OPT_TEMPERATURE, OPT_MAXTOKENS};

const static char *API_MODEL_IDS[] = {
    "gemini-2.5-flash",
    "gemini-2.5-flash-lite",
};

static SettingOption settings[] = {
    /* Label             {.actual Value}          {min, max, step}     toStringFunc   onEditFun*/
    {"Model: ",          {.iVal=MODEL_2_5_FLASH}, {0, MODEL_COUNT-1, 1}, ModelToString, EditCycle},
    {"Temperature: ",    {.fVal=1.0f},            {0.0f, 2.0f, 0.1f},    FloatToString, EditFloat},
    {"Max Tokens: ",     {.iVal=100},             {100, 1000, 1},        IntToString,   EditInt}
};

#define OPTION_COUNT sizeof(settings) / sizeof(SettingOption)

static int cursor = 0;

void Settings_Init() {
    
}

void EditInt(SettingOption *opt, int delta) {
    opt->value.iVal += (int)(opt->config.step * delta);
    if (opt->value.iVal > (int)opt->config.max) opt->value.iVal = (int)opt->config.max;
    if (opt->value.iVal < (int)opt->config.min) opt->value.iVal = (int)opt->config.min;
}

void EditFloat(SettingOption *opt, int delta) {
    opt->value.fVal += (opt->config.step * delta);
    if (opt->value.fVal > opt->config.max) opt->value.fVal = opt->config.max;
    if (opt->value.fVal < opt->config.min) opt->value.fVal = opt->config.min;
}

void EditBool(SettingOption *opt, int delta) {
    // Every input inverts the value
    opt->value.bVal = !opt->value.bVal;
}

/* To handle cycles lists like enums */
void EditCycle(SettingOption *opt, int delta) {
    opt->value.iVal += delta;

    // Wrap around
    if (opt->value.iVal > (int)opt->config.max) opt->value.iVal = 0;
    if (opt->value.iVal < (int)opt->config.min) opt->value.iVal = (int)opt->config.max;
}

void IntToString(SettingOption *opt, char *buf, size_t n) {
    snprintf(buf, n, "%d", opt->value.iVal);
}

void FloatToString(SettingOption *opt, char *buf, size_t n) {
    snprintf(buf, n, "%.1f", opt->value.fVal);
}

void BoolToString(SettingOption *opt, char *buf, size_t n) {
    snprintf(buf, n, "%s", opt->value.bVal ? "ON" : "OFF");
}

void ModelToString(SettingOption *opt, char *buf, size_t n) {
    const char *modelNames[] = {
        "Flash 2.5",
        "Flash Lite 2.5",
    };
    snprintf(buf, n, "%s", modelNames[opt->value.iVal]);
}

void Settings_Update(u32 kDown) {
   if (kDown & KEY_DOWN) {
       cursor++;
       if (cursor >= OPTION_COUNT) cursor = 0;
   }
   if (kDown & KEY_UP) {
       cursor--;
       if (cursor < 0) cursor = OPTION_COUNT - 1;
   }

   SettingOption *opt = &settings[cursor];
   if (kDown & KEY_RIGHT) {
       if (opt->onEdit) opt->onEdit(opt, 1);
   }

   if (kDown & KEY_LEFT) {
        if (opt->onEdit) opt->onEdit(opt, -1);
   }
}

void Settings_Draw() {
    /* Top Screen */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    char valBuf[32];
    for (int i = 0; i < OPTION_COUNT; i++) {
        SettingOption *opt = &settings[i];

        float y = 40 + (i * 25);

        if (i == cursor)
            R_DrawText(5, y, 1, ">", COLOR_TEXT_NORMAL);

        // Label
        R_DrawText(20, y, 1, opt->label, 
                (i == cursor) ? COLOR_TEXT_HIGHLIGHT : COLOR_TEXT_NORMAL);

        // Value
        if (opt->toString) {
            opt->toString(opt, valBuf, sizeof(valBuf));
            R_DrawText(200, y, 1, valBuf, COLOR_TEXT_HIGHLIGHT);
        }
    }

    /* Bottom Screen */
    // TODO Add description for each option 
}

const char* Settings_GetModel() {
    int val = settings[OPT_MODEL].value.iVal;

    // Array boundaries check
    if (val < 0 || val >= (sizeof(API_MODEL_IDS)/sizeof(char*))) {
        return API_MODEL_IDS[0]; // Return default model
    }

    return API_MODEL_IDS[val];
}

float Settings_GetTemperature() {
    return settings[OPT_TEMPERATURE].value.fVal;
}

int Settings_GetMaxTokens() {
    return settings[OPT_MAXTOKENS].value.iVal; 
}
