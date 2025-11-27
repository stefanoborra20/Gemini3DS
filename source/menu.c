#include "menu.h"
#include "renderer.h"

#define OPTION_COUNT 3 
static const char *options[] = {"Gemini", "API Key", "Settings"};
static int selectedOption = 0;

void Menu_Init() {
    selectedOption = 0;
}

MenuAction Menu_Update(u32 kDown) {
    if (kDown & KEY_DOWN) {
        selectedOption = (selectedOption + 1) % OPTION_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedOption--;
        if (selectedOption < 0) selectedOption = 0;
    }
    if (kDown & KEY_A) {
        if (selectedOption == 0) return MENU_ACTION_GOTO_GEMINI;
        if (selectedOption == 1) return MENU_ACTION_GOTO_APIKEY;
    }
    return MENU_ACTION_NONE;
}

void Menu_Draw() {
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    float startX = 100.0f;
    float startY = 100.0f;
    float lineSpacing = 30.0f;

    for (int i = 0; i < OPTION_COUNT; i++) {
        float y = startY + (i * lineSpacing);

        if (i == selectedOption) {
            R_DrawText(startX - 20, y, ">", COLOR_CURSOR);
            R_DrawText(startX, y, options[i], COLOR_TEXT_HIGHLIGHT);
        } else {
            R_DrawText(startX, y, options[i], COLOR_TEXT_NORMAL);
        }
    }
    
}
