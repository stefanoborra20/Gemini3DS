#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "renderer.h"
#include "menu.h"
#include "gemini_app.h"
#include "gemini_net.h"
#include "settings.h"
#include "mic_system.h"
#include "camera.h" 
#include "api_key_manager.h"

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
    ApiManager_Init();
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

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // --- UPDATE LOGIC ---
        switch (state) {
            case STATE_MENU:
                MenuAction action = Menu_Update(kDown);
                if (action == MENU_ACTION_GOTO_GEMINI) {
                    if (strlen(ApiManager_GetActiveKey()) > 0) {
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
            {
                CamMode mode_before_update = Cam_GetMode();
                GeminiApp_Update(kDown);

                if ((kDown & KEY_B) && mode_before_update == CAM_MODE_OFF) {
                    GeminiApp_Exit(); 
                    state = STATE_MENU;
                }
                break;
            }
                
            case STATE_APIKEY: 
                ApiManager_Update(kDown);
                if (kDown & KEY_B) {
                    state = STATE_MENU;
                }
                break;
                
            case STATE_SETTINGS:
                Settings_Update(kDown); 
                if (kDown & KEY_B) {
                    state = STATE_MENU;
                } 
                break;
        }

        // --- DRAW LOGIC ---
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
                ApiManager_Draw();
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
