#include "gemini_app.h"
#include "gemini_net.h"
#include "renderer.h"
#include "mic_system.h"
#include "camera.h"
#include "image_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    snprintf(responseText, MAX_RESPONSE_LEN, "://Gemini3DS/Insert promt");
    isThinking = false;
    scrollY = 0.0f;
    totalTextHeight = 0.0f;
    maxScrollY = 0.0f;
}

void GeminiApp_Update(u32 kDown, const char *apiKey) {
    if (isThinking) return;

    u32 kHeld = hidKeysHeld();
    u32 kUp = hidKeysUp();

    /* CAMERA CONTROLS */
    CamMode cam_mode = Cam_GetMode();
    if ((kDown & KEY_X) && cam_mode == CAM_MODE_OFF) {
        Cam_StartPreview();
        snprintf(responseText, MAX_RESPONSE_LEN, "Active preview. [A] Photo [B] Exit.");
        return;
    }

    if (cam_mode == CAM_MODE_PREVIEW) {
        Cam_UpdatePreview(); 

        if (kDown & KEY_A) {
            Cam_FreezeFrame(); 
        } else if (kDown & KEY_B) { 
            Cam_StopPreview(); 
        }
        return;
    }
    
    if (cam_mode == CAM_MODE_CAPTURED) {
        if (kDown & KEY_A) { // Confirm image
            isThinking = true; 
            ForceDrawStatus("::/Compressing image...");
            size_t jpegSize = 0;
            u8 *jpegBuffer = Encode_JPEG(Cam_GetBuffer(), 400, 240, 80, &jpegSize);

            if (jpegBuffer) {
                promtBuffer[0] = '\0';
                if (R_OpenKeyboard("Ask about this image...", promtBuffer, MAX_PROMT_LEN)) {
                    isThinking=true;
                    ForceDrawStatus("://Sending image & text to Gemini...");

                    const char *finalPromt = (strlen(promtBuffer) > 0) ? promtBuffer : "Describe this photo made from my Nintendo 3DS.";

                    Net_QueryGeminiImage(apiKey, finalPromt, jpegBuffer, jpegSize, responseText, MAX_RESPONSE_LEN);

                    R_ClearText(responseText);
                    isThinking=false;
                } else {
                    snprintf(responseText, MAX_RESPONSE_LEN, "Image prompt cancelled.");
                }
                free(jpegBuffer);
                
                
            } else {
                snprintf(responseText, MAX_RESPONSE_LEN, "Error: Failed to compress JPEG.");
            }
            
            Cam_StopPreview();
            isThinking = false;
        } else if (kDown & KEY_X) { // Retake image
            Cam_ResumePreview();
        } else if (kDown & KEY_B) { // Cancel and exit camera entirely
            Cam_StopPreview();
            snprintf(responseText, MAX_RESPONSE_LEN, "Camera cancelled.");
        }
        return;
    }

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
            
            ForceDrawStatus("://Thinking...");

            Net_QueryGemini(apiKey, promtBuffer, responseText, MAX_RESPONSE_LEN);
            R_ClearText(responseText);
            isThinking = false;
        }
    }

    if (kHeld & KEY_Y) Mic_StartRecording();

    if (Mic_IsRecording())  {
        Mic_Update();
        ForceDrawStatus("://Recording...");
    }

    if (kUp & KEY_Y) {
        Mic_StopRecording();

        /* Check if audio is at least ~= 0.5s */
        if (Mic_GetWavSize() > 16300) { 
            isThinking = true;

            ForceDrawStatus("://Sending audio...");

            const char *voicePrompt = 
            "Roleplay as a helpful voice assistant on a Nintendo 3DS. "
            "The attached audio is a direct question or statement from the user. "
            "Do not describe the audio. Do not transcribe what was said. "
            "Listen to the audio and reply directly and naturally to the user.";

            Net_QueryGeminiAudio(apiKey, voicePrompt, Mic_GetWavBuffer(), Mic_GetWavSize(), responseText, MAX_RESPONSE_LEN);
            R_ClearText(responseText);
            isThinking = false;
        }
    }
}

void GeminiApp_Draw() {
    CamMode cam_mode = Cam_GetMode();

    /* Top screen */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    if (cam_mode != CAM_MODE_OFF){
        R_DrawCameraFeed(Cam_GetBuffer());
        
        if (cam_mode == CAM_MODE_PREVIEW) {
            R_DrawText(10, 10, 1, "://Live Camera Preview", COLOR_TEXT_HIGHLIGHT);
        } else {
            R_DrawText(10, 10, 1, "://Photo Captured", COLOR_TEXT_HIGHLIGHT);
        }
    } else {

        if (isThinking) {
            R_DrawText(10, 10, 1, "://Thinking...", COLOR_TEXT_HIGHLIGHT);
        }
        else if (Mic_IsRecording()) {
            R_DrawText(10, 10, 1, "://Recording...", COLOR_TEXT_HIGHLIGHT);

            char audioSize[32];
            snprintf(audioSize, 32, "%lu Bytes", Mic_GetWavSize());
            R_DrawText(10, 30, 0.6f, audioSize, COLOR_TEXT_NORMAL);
        }
        else {

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
        }
    }

    /* Bottom screen */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    if (cam_mode == CAM_MODE_PREVIEW) {
        R_DrawText(10, 20, 1, "[A] Capture Photo", COLOR_TEXT_HIGHLIGHT);
        R_DrawText(10, 60, 1, "[B] Exit Camera", COLOR_TEXT_NORMAL);
    } 
    else if (cam_mode == CAM_MODE_CAPTURED) {
        R_DrawText(10, 20, 1, "[A] Use Image", COLOR_TEXT_HIGHLIGHT);
        R_DrawText(10, 60, 1, "[X] Retake Photo", COLOR_TEXT_NORMAL);
        R_DrawText(10, 100, 1, "[B] Exit Camera", COLOR_TEXT_NORMAL); 
    } 
    else {
        R_DrawText(10, 5, 1, "[A] Text Promt", COLOR_TEXT_NORMAL);
        R_DrawText(10, 35, 1, "[Y] Audio Promt (Hold)", COLOR_TEXT_NORMAL);
        R_DrawText(10, 65, 1, "[X] Open Camera", COLOR_TEXT_NORMAL);
        R_DrawText(10, 95, 1, "[B] Back", COLOR_TEXT_NORMAL);
    }   
}

void ForceDrawStatus(const char *message) {
    R_BeginFrame();
    
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);
    R_DrawText(10, 10, 1, message, COLOR_TEXT_HIGHLIGHT);

    R_EndFrame();

    gspWaitForVBlank();
}

void GeminiApp_Exit() {
    Cam_Exit();
}
