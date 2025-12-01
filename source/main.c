#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "renderer.h"
#include "menu.h"
#include "mem.h"
#include "gemini_app.h"
#include "gemini_net.h"

char currentApiKey[API_KEY_MAX_LEN] = "";

typedef enum {
    STATE_MENU,
    STATE_GEMINI,
    STATE_APIKEY
} State;


int init() {
    Net_Init();
    R_Init();
    GeminiApp_Init();
    return 0;
} 

void quit() {
    R_Exit();
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
                    state = STATE_GEMINI;
                    GeminiApp_Init();
                }
                else if (action == MENU_ACTION_GOTO_APIKEY) state = STATE_APIKEY; 
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
                    }
                } 
                break;

        }

        // Draw
        R_BeginFrame();

        switch (state) {
            case STATE_MENU:
                Menu_Draw();
                break;
            case STATE_APIKEY:
                R_SetTarget(SCREEN_BOTTOM);
                R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
                R_DrawText(10, 10, "Saved Api Key", COLOR_TEXT_NORMAL);

                R_DrawTextWrapped(10, 40, 380.0f, currentApiKey, COLOR_TEXT_HIGHLIGHT, NULL);
                
                R_DrawText(10, 100, "[A] Edit", COLOR_TEXT_NORMAL);
                R_DrawText(10, 130, "[B] Back", COLOR_TEXT_NORMAL);
                break;                
            case STATE_GEMINI:
                GeminiApp_Draw();
                break;
        }

        R_EndFrame();
    }
    
    quit();
    return 0;
}
