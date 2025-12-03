#include "gemini_app.h"
#include "gemini_net.h"
#include "renderer.h"
#include <string.h>
#include <stdio.h>

#define MAX_RESPONSE_LEN 8192
#define MAX_PROMT_LEN 256

static char responseText[MAX_RESPONSE_LEN];
static char promtBuffer[MAX_PROMT_LEN];
static bool isThinking = false;
static float scrollY;
static float totalTextHeight;
static const float scrollSpeed = 6.0f;
static float maxScrollY;

void GeminiApp_Init() {
    snprintf(responseText, MAX_RESPONSE_LEN, "GEMINI3DS: Write your promt.");
    isThinking = false;
    scrollY = 0.0f;
    totalTextHeight = 0.0f;
    maxScrollY = 0.0f;
}

void GeminiApp_Update(u32 kDown, const char *apiKey) {
    if (isThinking) return;

    u32 kHeld = hidKeysHeld();
    
    // Scroll
    if (kHeld & KEY_DOWN) scrollY += scrollSpeed;
    if (kHeld & KEY_UP) scrollY -= scrollSpeed;

    // Cap scrolling
    maxScrollY = totalTextHeight - SCREEN_TOP_HEIGHT + 20.0f; 
    if (maxScrollY < 0) maxScrollY = 0;
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScrollY) scrollY = maxScrollY;

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
    /* Top screen */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    float startY = 10.0f;
    float drawY = startY - scrollY;
    R_DrawTextWrapped(10.0f, drawY, SCREEN_TOP_WIDTH - 20.0f, responseText, COLOR_TEXT_NORMAL, &totalTextHeight); 

    // Draw side bar
    if (totalTextHeight > SCREEN_TOP_HEIGHT) {
        float barHeight = (SCREEN_TOP_HEIGHT / totalTextHeight) * SCREEN_TOP_HEIGHT;
        if (barHeight < 20) barHeight = 20;

        float barPos = (scrollY / (totalTextHeight - SCREEN_TOP_HEIGHT)) * (SCREEN_TOP_HEIGHT - barHeight);

        R_SetTarget(SCREEN_TOP);
        R_DrawRectSolid(SCREEN_TOP_WIDTH - 5.0f, barPos, 0.5f, 4.0f, barHeight, COLOR_SCROLL_BAR);
    }

    /* Bottom screen */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);
    
    R_DrawText(10, 10, "Commands:", COLOR_TEXT_HIGHLIGHT);
    R_DrawText(10, 40, "[A] New Promt", COLOR_TEXT_NORMAL);
    R_DrawText(10, 60, "[B] Back", COLOR_TEXT_NORMAL);
   
    if (isThinking) {
        R_DrawText(10, 150, "Loading...", COLOR_TEXT_HIGHLIGHT);
    } 

}
