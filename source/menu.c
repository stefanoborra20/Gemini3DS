#include "menu.h"
#include "renderer.h"
#include "settings.h"
#include "api_key_manager.h"
#include <stdio.h>
#include <string.h>

#define OPTION_COUNT 3

extern bool printApiKeyErr;

static const char *options[] = {"CHAT", "KEYS", "SETTINGS"};
static int selectedOption = 0;

static MenuAction Activate(int option) {
    if (option == 0) return MENU_ACTION_GOTO_GEMINI;
    if (option == 1) return MENU_ACTION_GOTO_APIKEY;
    if (option == 2) return MENU_ACTION_GOTO_SETTINGS;
    return MENU_ACTION_NONE;
}

void Menu_Init() {
    selectedOption = 0;
}

MenuAction Menu_Update(u32 kDown) {
    touchPosition touch;
    bool touchDown = false;
    bool touchUp = false;
    R_UpdateTouch(&touch, &touchDown, &touchUp);

    if (touchDown) {
        for (int i = 0; i < OPTION_COUNT; i++) {
            float y = 34.0f + i * 62.0f;
            if (R_TouchInRect(&touch, 8.0f, y, 304.0f, 54.0f)) {
                selectedOption = i;
                return Activate(i);
            }
        }
    }

    if (kDown & KEY_DOWN) {
        selectedOption = (selectedOption + 1) % OPTION_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedOption = (selectedOption + OPTION_COUNT - 1) % OPTION_COUNT;
    }
    if (kDown & KEY_A) {
        return Activate(selectedOption);
    }

    return MENU_ACTION_NONE;
}

static void Draw_HomeArt(void) {
    static const char *art[16] = {
        ".......##.......",
        ".......##.......",
        "......####......",
        "......####......",
        ".....######.....",
        "....########....",
        "...##########...",
        "..############..",
        "..############..",
        "....########....",
        "....########....",
        ".....######.....",
        "......####......",
        "......####......",
        ".......##.......",
        ".......##......."
    };

    const float cell = 9.0f;
    const float artW = 16.0f * cell;
    const float x0 = (SCREEN_TOP_WIDTH - artW) * 0.5f;
    const float y0 = (SCREEN_TOP_HEIGHT - artW) * 0.5f;

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            if (art[row][col] == '#') {
                R_DrawRectSolid(x0 + col * cell, y0 + row * cell, 0.5f,
                                cell, cell, COLOR_ACCENT);
            }
        }
    }

    /* Bright pixel core. */
    R_DrawRectSolid(x0 + 7.0f * cell, y0 + 7.0f * cell, 0.5f,
                    2.0f * cell, 2.0f * cell, COLOR_TEXT_HIGHLIGHT);

    R_DrawIcon(x0 - 30.0f, y0 + 10.0f, 18.0f, ICON_SPARKLE, COLOR_TEXT_DIM);
    R_DrawIcon(x0 + artW + 14.0f, y0 + artW - 36.0f, 12.0f, ICON_SPARKLE, COLOR_ACCENT);
}

void Menu_Draw() {
    /* ------------------------------------------------------------------ */
    /* Top screen: pixel art only                                         */
    /* ------------------------------------------------------------------ */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);
    Draw_HomeArt();

    /* ------------------------------------------------------------------ */
    /* Bottom screen: just the menu                                       */
    /* ------------------------------------------------------------------ */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    for (int i = 0; i < OPTION_COUNT; i++) {
        float rowY = 34.0f + i * 62.0f;
        bool selected = (i == selectedOption);
        bool needsKey = (printApiKeyErr && i == 1);

        Color color = selected ? COLOR_ACCENT
                               : (needsKey ? COLOR_ALERT : COLOR_TEXT_NORMAL);

        float textH = R_TextHeight(options[i], 0.9f);
        float textW = R_TextWidth(options[i], 0.9f);
        float textY = rowY + (54.0f - textH) * 0.5f;

        R_DrawTextCentered(0.0f, textY, SCREEN_BOTTOM_WIDTH, 0.9f, options[i], color);

        if (selected || needsKey) {
            float lineW = textW;
            float lineX = (SCREEN_BOTTOM_WIDTH - lineW) * 0.5f;
            R_DrawRectSolid(lineX, textY + textH + 4.0f, 0.5f, lineW, 2.0f,
                            selected ? COLOR_ACCENT : COLOR_ALERT);
        }
    }
}
