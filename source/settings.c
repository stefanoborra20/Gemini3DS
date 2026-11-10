#include "settings.h"
#include "renderer.h"
#include <stdio.h>
#include <string.h>

void IntToString(SettingOption*, char*, size_t);
void FloatToString(SettingOption*, char*, size_t);
void BoolToString(SettingOption*, char*, size_t);
void ModelToString(SettingOption*, char*, size_t);
void ThemeToString(SettingOption*, char*, size_t);

void EditInt(SettingOption*, int);
void EditFloat(SettingOption*, int);
void EditBool(SettingOption*, int);
void EditCycle(SettingOption*, int);
void EditTheme(SettingOption*, int);

enum {OPT_MODEL=0, OPT_TEMPERATURE, OPT_MAXTOKENS, OPT_THEME};

const static char *API_MODEL_IDS[] = {
    "gemini-3.5-flash",
    "gemini-3.1-flash-lite",
    "gemini-2.5-flash",
    "gemini-2.5-flash-lite",
};

const static char *MODEL_LABELS[] = {
    "FLASH 3.5",
    "LITE 3.1",
    "FLASH 2.5",
    "LITE 2.5",
};

const static char *settingDescriptions[] = {
    "Please note that some models featured here may be discontinued.",
    "Control response creativity. The higher the more creative.",
    "Maximum generated token limit. Note that one token not always equals to one word.",
    "Change the interface palette. The choice is saved to gemini_ui.cfg.",
};

static SettingOption settings[] = {
    /* Label          Description                            {.actual Value}          {min, max, step}       toStringFunc   onEditFun*/
    {"MODEL",         &settingDescriptions[OPT_MODEL],       {.iVal=MODEL_3_5_FLASH}, {0, MODEL_COUNT-1, 1}, ModelToString, EditCycle},
    {"TEMPERATURE",   &settingDescriptions[OPT_TEMPERATURE], {.fVal=1.0f},            {0.0f, 2.0f, 0.1f},    FloatToString, EditFloat},
    {"MAX TOKENS",    &settingDescriptions[OPT_MAXTOKENS],   {.iVal=1000},            {100, 3000, 100},      IntToString,   EditInt},
    {"THEME",         &settingDescriptions[OPT_THEME],       {.iVal=0},               {0, R_THEME_COUNT-1, 1}, ThemeToString, EditTheme},
};

#define OPTION_COUNT ((int)(sizeof(settings) / sizeof(SettingOption)))

static int cursor = 0;

static void SaveTheme(void) {
    FILE *f = fopen("gemini_ui.cfg", "w");
    if (f) {
        fprintf(f, "%d\n", R_GetThemeIndex());
        fclose(f);
    }
}

void Settings_Init() {
    FILE *f = fopen("gemini_ui.cfg", "r");
    if (f) {
        int theme = 0;
        if (fscanf(f, "%d", &theme) == 1) R_SetTheme(theme);
        fclose(f);
    }

    settings[OPT_THEME].value.iVal = R_GetThemeIndex();
    cursor = 0;
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
    (void)delta;
    opt->value.bVal = !opt->value.bVal;
}

/* To handle cycles lists like enums */
void EditCycle(SettingOption *opt, int delta) {
    opt->value.iVal += delta;

    if (opt->value.iVal > (int)opt->config.max) opt->value.iVal = 0;
    if (opt->value.iVal < (int)opt->config.min) opt->value.iVal = (int)opt->config.max;
}

void EditTheme(SettingOption *opt, int delta) {
    int next = R_GetThemeIndex() + delta;

    if (next < 0) next = R_THEME_COUNT - 1;
    if (next >= R_THEME_COUNT) next = 0;

    R_SetTheme(next);
    opt->value.iVal = next;
    SaveTheme();
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
    int index = opt->value.iVal;
    if (index < 0 || index >= MODEL_COUNT) index = 0;
    snprintf(buf, n, "%s", MODEL_LABELS[index]);
}

void ThemeToString(SettingOption *opt, char *buf, size_t n) {
    snprintf(buf, n, "%s", R_GetThemeName(opt->value.iVal));
}

static void EditCurrent(int delta) {
    SettingOption *opt = &settings[cursor];
    if (opt->onEdit) opt->onEdit(opt, delta);
}

bool Settings_Update(u32 kDown) {
    touchPosition touch;
    bool touchDown = false;
    bool touchUp = false;
    R_UpdateTouch(&touch, &touchDown, &touchUp);

    if (touchDown) {
        for (int i = 0; i < OPTION_COUNT; i++) {
            if (R_TouchInRect(&touch, 8.0f + i * 77.0f, 28.0f, 72.0f, 26.0f)) {
                cursor = i;
                return false;
            }
        }

        if (R_TouchInRect(&touch, 14.0f, 126.0f, 40.0f, 32.0f)) {
            EditCurrent(-1);
            return false;
        }

        if (R_TouchInRect(&touch, 266.0f, 126.0f, 40.0f, 32.0f)) {
            EditCurrent(1);
            return false;
        }

        if (R_TouchInRect(&touch, 8.0f, 172.0f, 304.0f, 34.0f)) {
            return true;
        }
    }

    if (kDown & KEY_DOWN) {
        cursor = (cursor + 1) % OPTION_COUNT;
    }
    if (kDown & KEY_UP) {
        cursor = (cursor + OPTION_COUNT - 1) % OPTION_COUNT;
    }
    if (kDown & KEY_RIGHT) {
        EditCurrent(1);
    }
    if (kDown & KEY_LEFT) {
        EditCurrent(-1);
    }
    if (kDown & KEY_B) {
        return true;
    }

    return false;
}

void Settings_Draw() {
    char valBuf[32];

    /* ------------------------------------------------------------------ */
    /* Top screen                                                         */
    /* ------------------------------------------------------------------ */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    for (int i = 0; i < OPTION_COUNT; i++) {
        SettingOption *opt = &settings[i];
        float y = 32.0f + i * 42.0f;
        bool selected = (i == cursor);

        if (selected) {
            R_DrawRectSolid(14.0f, y + 7.0f, 0.5f, 3.0f, 22.0f, COLOR_ACCENT);
        }

        Color labelColor = selected ? COLOR_TEXT_HIGHLIGHT : COLOR_TEXT_DIM;
        Color valueColor = selected ? COLOR_ACCENT : COLOR_TEXT_NORMAL;

        R_DrawText(26.0f, y + 8.0f, 0.55f, opt->label, labelColor);

        if (opt->toString) {
            opt->toString(opt, valBuf, sizeof(valBuf));
            R_DrawTextRight(388.0f, y + 8.0f, 0.6f, valBuf, valueColor);
        }
    }

    /* ------------------------------------------------------------------ */
    /* Bottom screen                                                      */
    /* ------------------------------------------------------------------ */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    const char *tabLabels[] = {"MODEL", "TEMP", "TOKENS", "THEME"};

    for (int i = 0; i < OPTION_COUNT; i++) {
        float x = 8.0f + i * 77.0f;
        float y = 28.0f;
        bool selected = (i == cursor);

        if (selected) {
            R_DrawRectSolid(x, y + 24.0f, 0.5f, 72.0f, 2.0f, COLOR_ACCENT);
        }

        R_DrawTextCentered(x, y + 7.0f, 72.0f, 0.45f, tabLabels[i],
                           selected ? COLOR_TEXT_HIGHLIGHT : COLOR_TEXT_DIM);
    }

    R_DrawTextCentered(0.0f, 82.0f, SCREEN_BOTTOM_WIDTH, 0.5f,
                       settings[cursor].label, COLOR_TEXT_DIM);

    /* Left / right value controls */
    R_DrawTextCentered(14.0f, 132.0f, 40.0f, 0.8f, "<", COLOR_ACCENT);
    R_DrawTextCentered(266.0f, 132.0f, 40.0f, 0.8f, ">", COLOR_ACCENT);

    if (settings[cursor].toString) {
        settings[cursor].toString(&settings[cursor], valBuf, sizeof(valBuf));
        R_DrawTextCentered(60.0f, 132.0f, 200.0f, 0.75f,
                           valBuf, COLOR_TEXT_HIGHLIGHT);
    }

    R_DrawButton(8.0f, 172.0f, 304.0f, 34.0f,
                 "B", "BACK", ICON_NONE, BTN_NORMAL);
}

const char* Settings_GetModel() {
    int val = settings[OPT_MODEL].value.iVal;

    if (val < 0 || val >= (int)(sizeof(API_MODEL_IDS) / sizeof(char*))) {
        return API_MODEL_IDS[0];
    }

    return API_MODEL_IDS[val];
}

const char* Settings_GetModelLabel() {
    int val = settings[OPT_MODEL].value.iVal;

    if (val < 0 || val >= MODEL_COUNT) {
        return MODEL_LABELS[0];
    }

    return MODEL_LABELS[val];
}

float Settings_GetTemperature() {
    return settings[OPT_TEMPERATURE].value.fVal;
}

int Settings_GetMaxTokens() {
    return settings[OPT_MAXTOKENS].value.iVal;
}
