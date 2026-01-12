#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "renderer.h"
#include "menu.h"
#include "mem.h"
#include "gemini_app.h"
#include "gemini_net.h"
#include "settings.h"
#include "mic_system.h"

char currentApiKey[API_KEY_MAX_LEN] = "";
bool printApiKeyErr = false;

typedef enum {
    STATE_MENU,
    STATE_GEMINI,
    STATE_APIKEY,
    STATE_SETTINGS
} State;


int init() {
    Net_Init();
    R_Init();
    Mic_Init();
    Settings_Init();
    GeminiApp_Init();
    return 0;
} 

void quit() {
    R_Exit();
    Mic_Exit();
    Net_Exit();
}

int main(int argc, char **argv) {
    if (init() != 0) quit();
    
    State state = STATE_MENU;
    Mem_LoadApiKey(currentApiKey, API_KEY_MAX_LEN);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        switch (state) {
            case STATE_MENU:
                MenuAction action = Menu_Update(kDown);
                if (action == MENU_ACTION_GOTO_GEMINI) {
                    // Check if Api Key was inserted
                    if (strcmp(currentApiKey, "") != 0) {
                        state = STATE_GEMINI;
                        GeminiApp_Init();
                    } else {
                        printApiKeyErr = true;
                    }
                }
                else if (action == MENU_ACTION_GOTO_APIKEY) {
                    state = STATE_APIKEY; 
                } 
                else if (action == MENU_ACTION_GOTO_SETTINGS) {
                    state = STATE_SETTINGS;
                }
                break;
            case STATE_GEMINI:
                GeminiApp_Update(kDown, currentApiKey);

                if (kDown & KEY_B) state = STATE_MENU;
                break;
            case STATE_APIKEY: 
                if (kDown & KEY_B) {
                    state = STATE_MENU;
                }
                if (kDown & KEY_A) {
                    char tempBuffer[API_KEY_MAX_LEN];
                    strncpy(tempBuffer, currentApiKey, API_KEY_MAX_LEN);
                    if (strcmp(tempBuffer, "No Api Key") == 0) tempBuffer[0] = '\0'; 

                    if (R_OpenKeyboard("Insert your Gemini API Key", tempBuffer, API_KEY_MAX_LEN)) {
                        strncpy(currentApiKey, tempBuffer, API_KEY_MAX_LEN);
                        Mem_SaveApiKey(currentApiKey);
                        if (printApiKeyErr) printApiKeyErr = false;
                    }
                } 
                break;
            case STATE_SETTINGS:
                Settings_Update(kDown); 

                if (kDown & KEY_B) {
                    state = STATE_MENU;
                } 
                break;

        }

        // Draw
        R_BeginFrame();

        switch (state) {
            case STATE_MENU:
                Menu_Draw();

                if (printApiKeyErr) {
                    R_SetTarget(SCREEN_BOTTOM);
                    R_DrawText(10, 5, 1, "No api key inserted", COLOR_TEXT_HIGHLIGHT);
                }
                break;
            case STATE_APIKEY:
                R_SetTarget(SCREEN_BOTTOM);
                R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
                R_DrawText(10, 10, 1, "Saved Api Key", COLOR_TEXT_NORMAL);

                R_DrawText(10, 40, 0.5f, currentApiKey, COLOR_TEXT_HIGHLIGHT);
                
                R_DrawText(10, 100, 1, "[A] Edit", COLOR_TEXT_NORMAL);
                R_DrawText(10, 130, 1, "[B] Back", COLOR_TEXT_NORMAL);
                break;                
            case STATE_GEMINI:
                GeminiApp_Draw();
                break;
            case STATE_SETTINGS:
                Settings_Draw();
                break;
        }

        R_EndFrame();
    }
    
    quit();
    return 0;
}
