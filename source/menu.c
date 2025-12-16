#include "menu.h"
#include "renderer.h"

#define OPTION_COUNT 3 

// Logo will be replaced with some ASCII art
const char *logo = 
    "< GEMINI-3DS >";

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
        if (selectedOption == 2) return MENU_ACTION_GOTO_SETTINGS;
    }
    return MENU_ACTION_NONE;
}

void Menu_Draw() {
    // Screens Clear
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    /* Top Screen */
    float startX = SCREEN_TOP_WIDTH / 2 - 50.0f; 
    float startY = SCREEN_TOP_HEIGHT / 2 - 20.0f; 
    float lineSpacing = 30.0f;

    R_DrawText(SCREEN_TOP_WIDTH / 2 - 90.0f, 5, 0.8f, logo, COLOR_TEXT_HIGHLIGHT);

    for (int i = 0; i < OPTION_COUNT; i++) {
        float y = startY + (i * lineSpacing);

        if (i == selectedOption) {
            R_DrawText(startX - 20, y, 1, ">", COLOR_CURSOR);
            R_DrawText(startX, y, 1, options[i], COLOR_TEXT_HIGHLIGHT);
        } else {
            R_DrawText(startX, y, 1, options[i], COLOR_TEXT_NORMAL);
        }
    }  
}

