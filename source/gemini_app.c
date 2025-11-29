#include "gemini_app.h"
#include "gemini_net.h"
#include "renderer.h"
#include <string.h>
#include <stdio.h>

//#define MAX_RESPONSE_LEN 2048
#define MAX_RESPONSE_LEN 8192
#define MAX_PROMT_LEN 256

static char responseText[MAX_RESPONSE_LEN];
static char promtBuffer[MAX_PROMT_LEN];
static bool isThinking = false;

void GeminiApp_Init() {
    snprintf(responseText, MAX_RESPONSE_LEN, "GEMINI3DS: Write your promt.");
    isThinking = false;
}

void GeminiApp_Update(u32 kDown, const char *apiKey) {
    if (isThinking) return;

    if (kDown & KEY_A) {
        promtBuffer[0] = '\0';

        if (R_OpenKeyboard("Ask Gemini...", promtBuffer, MAX_PROMT_LEN)) {
            isThinking = true;

            R_BeginFrame();
            R_SetTarget(SCREEN_TOP);
            R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);
            R_DrawText(10, 10, "Gemini is thinking...", COLOR_TEXT_HIGHLIGHT);
            R_EndFrame();

            Net_QueryGemini(apiKey, promtBuffer, responseText, MAX_RESPONSE_LEN);

            isThinking = false;
        }
    }
}

void GeminiApp_Draw() {
    // Top screen
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    R_DrawTextWrapped(0, 0, SCREEN_TOP_WIDTH, responseText, COLOR_TEXT_NORMAL); 

    // Bottom screen
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
    
    R_DrawText(10, 10, "Commands:", COLOR_TEXT_HIGHLIGHT);
    R_DrawText(10, 40, "[A] New Promt", COLOR_TEXT_NORMAL);
    R_DrawText(10, 60, "[B] Back", COLOR_TEXT_NORMAL);
   
    if (isThinking) {
        R_DrawText(10, 150, "Loading...", COLOR_TEXT_HIGHLIGHT);
    } 

}
