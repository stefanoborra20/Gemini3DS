#include "gemini_app.h"
#include "gemini_net.h"
#include "renderer.h"
#include "mic_system.h"
#include "camera.h"
#include "image_utils.h"
#include "settings.h"
#include "api_key_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_RESPONSE_LEN 8192
#define MAX_PROMPT_LEN 256

/* Top screen layout */
#define BODY_X 6.0f
#define BODY_Y 26.0f
#define BODY_W 388.0f
#define BODY_H 190.0f
#define TEXT_X 14.0f
#define TEXT_Y 32.0f
#define TEXT_W 362.0f
#define TEXT_CLIP_BOTTOM 214.0f
#define TEXT_VIEW_H 182.0f
#define ROLE_W 30.0f
#define SCROLL_X 384.0f
#define SCROLL_H 182.0f

/* Bottom screen layout */
#define BOT_PROMPT_X 8.0f
#define BOT_PROMPT_Y 30.0f
#define BOT_PROMPT_W 304.0f
#define BOT_PROMPT_H 38.0f
#define BOT_BTN_W 148.0f
#define BOT_BTN_H 56.0f
#define BOT_BTN_X1 8.0f
#define BOT_BTN_X2 164.0f
#define BOT_BTN_Y1 78.0f
#define BOT_BTN_Y2 142.0f

/* Camera bottom-screen buttons */
#define CAM_PREVIEW_SHUTTER_Y 40.0f
#define CAM_PREVIEW_EXIT_Y 106.0f
#define CAM_CAPTURED_USE_Y 34.0f
#define CAM_CAPTURED_RETAKE_Y 90.0f
#define CAM_CAPTURED_EXIT_Y 146.0f
#define CAM_BTN_H 48.0f

static char responseText[MAX_RESPONSE_LEN];
static char promptBuffer[MAX_PROMPT_LEN];
static char lastPrompt[MAX_PROMPT_LEN];
static bool isThinking = false;
static float scrollY;
static float totalTextHeight;
static const float scrollSpeed = 6.0f;
static float maxScrollY;
static bool wasTalkHeld = false;
static bool talkTouchActive = false;
static int recFrame = 0;

void GeminiApp_Init() {
    snprintf(responseText, MAX_RESPONSE_LEN,
             "Welcome to Gemini3DS. Press A to write a prompt, hold Y to talk, or press X to use the camera.");
    lastPrompt[0] = '\0';
    isThinking = false;
    scrollY = 0.0f;
    totalTextHeight = 0.0f;
    maxScrollY = 0.0f;
    wasTalkHeld = false;
    talkTouchActive = false;
    recFrame = 0;

    Net_SetStatusCallback(ForceDrawStatus);
}

void GeminiApp_Exit() {
    Cam_Exit();
}

/* -------------------------------------------------------------------------
 * Status screens
 * ---------------------------------------------------------------------- */

static void Draw_WorkingScreen(const char *message) {
    if (!message || !*message) message = "://WORKING";

    /* Top screen */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    R_DrawIcon(SCREEN_TOP_WIDTH / 2 - 10.0f, 58.0f, 20.0f,
               ICON_SPARKLE, COLOR_ACCENT);
    R_DrawTextCentered(0.0f, 104.0f, SCREEN_TOP_WIDTH, 0.7f,
                       message, COLOR_TEXT_HIGHLIGHT);

    /* Bottom screen */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    R_DrawTextCentered(0.0f, 110.0f, SCREEN_BOTTOM_WIDTH, 0.5f,
                       "PLEASE WAIT", COLOR_TEXT_DIM);
}

static void Draw_RecordingScreen(void) {
    recFrame++;

    u32 bytes = Mic_GetWavSize();
    u32 audioBytes = (bytes > 44) ? (bytes - 44) : 0;
    u32 tenths = audioBytes / 3272;
    char timeBuf[32];
    snprintf(timeBuf, sizeof(timeBuf), "%lu.%lu S",
             (unsigned long)(tenths / 10), (unsigned long)(tenths % 10));

    float progress = (float)audioBytes / (32720.0f * 10.0f);
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    /* Top screen */
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    R_DrawIcon(SCREEN_TOP_WIDTH / 2 - 12.0f, 58.0f, 24.0f, ICON_MIC, COLOR_ALERT);

    R_DrawRectSolid(SCREEN_TOP_WIDTH / 2 - 34.0f, 100.0f, 0.5f, 6.0f, 6.0f,
                    (recFrame / 15) % 2 ? COLOR_ALERT : COLOR_TEXT_DIM);
    R_DrawText(SCREEN_TOP_WIDTH / 2 - 22.0f, 96.0f, 0.8f, "REC", COLOR_ALERT);
    R_DrawTextCentered(0.0f, 124.0f, SCREEN_TOP_WIDTH, 0.7f,
                       timeBuf, COLOR_TEXT_HIGHLIGHT);

    R_DrawRectSolid(80.0f, 164.0f, 0.5f, 240.0f, 3.0f, COLOR_SCROLL_TRACK);
    R_DrawRectSolid(80.0f, 164.0f, 0.5f, 240.0f * progress, 3.0f, COLOR_ALERT);

    /* Bottom screen */
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    R_DrawTextCentered(0.0f, 110.0f, SCREEN_BOTTOM_WIDTH, 0.5f,
                       "RELEASE TO SEND", COLOR_TEXT_DIM);
}

void ForceDrawStatus(const char *message) {
    R_BeginFrame();

    if (Mic_IsRecording()) {
        Draw_RecordingScreen();
    } else {
        Draw_WorkingScreen(message);
    }

    R_EndFrame();
    gspWaitForVBlank();
}

/* -------------------------------------------------------------------------
 * Input handlers
 * ---------------------------------------------------------------------- */

static void Handle_CameraState(u32 kDown, CamMode cam_mode, bool touchDown,
                               const touchPosition *touch) {
    if (cam_mode == CAM_MODE_PREVIEW) {
        Cam_UpdatePreview();

        bool shutter = (kDown & KEY_A) ||
                       (touchDown && R_TouchInRect(touch, 8.0f, CAM_PREVIEW_SHUTTER_Y,
                                                   304.0f, 56.0f));
        bool exit = (kDown & KEY_B) ||
                    (touchDown && R_TouchInRect(touch, 8.0f, CAM_PREVIEW_EXIT_Y,
                                                304.0f, 56.0f));

        if (shutter) {
            Cam_FreezeFrame();
        } else if (exit) {
            Cam_StopPreview();
        }
        return;
    }

    if (cam_mode == CAM_MODE_CAPTURED) {
        bool use = (kDown & KEY_A) ||
                   (touchDown && R_TouchInRect(touch, 8.0f, CAM_CAPTURED_USE_Y,
                                               304.0f, CAM_BTN_H));
        bool retake = (kDown & KEY_X) ||
                      (touchDown && R_TouchInRect(touch, 8.0f, CAM_CAPTURED_RETAKE_Y,
                                                  304.0f, CAM_BTN_H));
        bool exit = (kDown & KEY_B) ||
                    (touchDown && R_TouchInRect(touch, 8.0f, CAM_CAPTURED_EXIT_Y,
                                                304.0f, CAM_BTN_H));

        if (use) {
            isThinking = true;
            ForceDrawStatus("://CONVERTING JPEG");

            size_t jpegSize = 0;
            u8 *jpegBuffer = Encode_JPEG(Cam_GetBuffer(), 400, 240, 80, &jpegSize);

            if (jpegBuffer) {
                promptBuffer[0] = '\0';

                if (R_OpenKeyboard("Ask about this image...", promptBuffer, MAX_PROMPT_LEN)) {
                    if (promptBuffer[0]) {
                        snprintf(lastPrompt, sizeof(lastPrompt), "[IMAGE] %s", promptBuffer);
                    } else {
                        snprintf(lastPrompt, sizeof(lastPrompt), "[IMAGE]");
                    }

                    isThinking = true;
                    ForceDrawStatus("://SENDING IMAGE");

                    const char *finalPrompt = (strlen(promptBuffer) > 0)
                                                  ? promptBuffer
                                                  : "Describe this photo made from my Nintendo 3DS.";

                    Net_QueryGeminiImage(finalPrompt, jpegBuffer, jpegSize,
                                         responseText, MAX_RESPONSE_LEN);
                    R_ClearText(responseText);
                    scrollY = 0.0f;
                    isThinking = false;
                } else {
                    snprintf(responseText, MAX_RESPONSE_LEN, "Image prompt cancelled.");
                    lastPrompt[0] = '\0';
                    scrollY = 0.0f;
                }

                free(jpegBuffer);
            } else {
                snprintf(responseText, MAX_RESPONSE_LEN, "Error: Failed to compress JPEG.");
                lastPrompt[0] = '\0';
                scrollY = 0.0f;
            }

            Cam_StopPreview();
            R_SuppressTouch();
            isThinking = false;
        } else if (retake) {
            Cam_ResumePreview();
        } else if (exit) {
            Cam_StopPreview();
            snprintf(responseText, MAX_RESPONSE_LEN, "Camera cancelled.");
            lastPrompt[0] = '\0';
            scrollY = 0.0f;
        }
    }
}

static void Handle_Scrolling(u32 kHeld) {
    if (kHeld & KEY_DOWN) scrollY += scrollSpeed;
    if (kHeld & KEY_UP) scrollY -= scrollSpeed;

    maxScrollY = totalTextHeight - TEXT_VIEW_H;
    if (maxScrollY < 0.0f) maxScrollY = 0.0f;
    if (scrollY < 0.0f) scrollY = 0.0f;
    if (scrollY > maxScrollY) scrollY = maxScrollY;
}

static void Handle_TextPrompt(bool pressed) {
    if (!pressed) return;

    promptBuffer[0] = '\0';

    if (R_OpenKeyboard("Ask Gemini...", promptBuffer, MAX_PROMPT_LEN)) {
        snprintf(lastPrompt, sizeof(lastPrompt), "%s", promptBuffer);

        isThinking = true;
        ForceDrawStatus("://THINKING");

        Net_QueryGemini(promptBuffer, responseText, MAX_RESPONSE_LEN);
        R_ClearText(responseText);
        scrollY = 0.0f;
        isThinking = false;
        R_SuppressTouch();
    }
}

static void Handle_AudioPrompt(bool held, bool up) {
    if (held && !Mic_IsRecording()) {
        Mic_StartRecording();
    }

    if (Mic_IsRecording()) {
        Mic_Update();
    }

    if (!up) return;

    if (!Mic_IsRecording()) return;

    Mic_StopRecording();

    /* Check if audio is at least ~= 0.5s */
    if (Mic_GetWavSize() > 16300) {
        snprintf(lastPrompt, sizeof(lastPrompt), "[VOICE]");

        isThinking = true;
        ForceDrawStatus("://SENDING AUDIO");

        const char *voicePrompt =
            "Roleplay as a helpful voice assistant on a Nintendo 3DS. "
            "The attached audio is a direct question or statement from the user. "
            "Do not describe the audio. Do not transcribe what was said. "
            "Listen to the audio and reply directly and naturally to the user.";

        Net_QueryGeminiAudio(voicePrompt, Mic_GetWavBuffer(), Mic_GetWavSize(),
                             responseText, MAX_RESPONSE_LEN);
        R_ClearText(responseText);
        scrollY = 0.0f;
        isThinking = false;
        R_SuppressTouch();
    } else {
        snprintf(responseText, MAX_RESPONSE_LEN,
                 "Voice prompt too short. Hold Y a little longer.");
        lastPrompt[0] = '\0';
        scrollY = 0.0f;
    }
}

bool GeminiApp_Update(u32 kDown) {
    if (isThinking) return false;

    u32 kHeld = hidKeysHeld();

    touchPosition touch;
    bool touchDown = false;
    bool touchUp = false;
    bool touching = R_UpdateTouch(&touch, &touchDown, &touchUp);
    (void)touchUp;

    bool inTalk = R_TouchInRect(&touch, BOT_BTN_X2, BOT_BTN_Y1, BOT_BTN_W, BOT_BTN_H);
    if (touchDown && inTalk) talkTouchActive = true;
    if (!touching || !inTalk) talkTouchActive = false;

    bool talkHeld = (kHeld & KEY_Y) || talkTouchActive;
    bool talkUp = wasTalkHeld && !talkHeld;
    wasTalkHeld = talkHeld;

    CamMode cam_mode = Cam_GetMode();

    if (cam_mode != CAM_MODE_OFF) {
        Handle_CameraState(kDown, cam_mode, touchDown, &touch);
        return false;
    }

    if (Mic_IsRecording()) {
        Handle_AudioPrompt(talkHeld, talkUp);
        return false;
    }

    bool ask = (kDown & KEY_A) ||
               (touchDown && R_TouchInRect(&touch, BOT_BTN_X1, BOT_BTN_Y1,
                                            BOT_BTN_W, BOT_BTN_H));
    bool cam = (kDown & KEY_X) ||
               (touchDown && R_TouchInRect(&touch, BOT_BTN_X1, BOT_BTN_Y2,
                                            BOT_BTN_W, BOT_BTN_H));
    bool back = (kDown & KEY_B) ||
                (touchDown && R_TouchInRect(&touch, BOT_BTN_X2, BOT_BTN_Y2,
                                             BOT_BTN_W, BOT_BTN_H));

    if (back) return true;
    if (cam) {
        Cam_StartPreview();
        return false;
    }

    Handle_Scrolling(kHeld);
    Handle_TextPrompt(ask);
    Handle_AudioPrompt(talkHeld, talkUp);

    return false;
}

/* -------------------------------------------------------------------------
 * Drawing
 * ---------------------------------------------------------------------- */

static void Draw_CameraOverlay(CamMode cam_mode) {
    const char *status = (cam_mode == CAM_MODE_PREVIEW) ? "LIVE" : "CAPTURED";

    R_DrawRectSolid(12.0f, 12.0f, 0.5f, 6.0f, 6.0f,
                    cam_mode == CAM_MODE_PREVIEW ? COLOR_ALERT : COLOR_ACCENT);
    R_DrawText(24.0f, 8.0f, 0.5f, status, COLOR_TEXT_HIGHLIGHT);
}

static void Draw_TopScreen(CamMode cam_mode) {
    R_SetTarget(SCREEN_TOP);
    R_ClearScreen(SCREEN_TOP, COLOR_BACKGROUND);

    if (cam_mode != CAM_MODE_OFF) {
        R_DrawCameraFeed(Cam_GetBuffer());
        Draw_CameraOverlay(cam_mode);
        return;
    }

    float promptHeight = 0.0f;
    float responseHeight = 0.0f;
    float cursorY = TEXT_Y - scrollY;

    if (lastPrompt[0]) {
        if (cursorY >= TEXT_Y && cursorY + 16.0f <= TEXT_CLIP_BOTTOM) {
            R_DrawText(TEXT_X, cursorY + 2.0f, 0.5f, "YOU", COLOR_ACCENT);
        }

        R_DrawTextWrappedClipped(TEXT_X + ROLE_W, cursorY, TEXT_W - ROLE_W, 0.7f,
                                 lastPrompt, COLOR_TEXT_NORMAL, 2.0f,
                                 TEXT_Y, TEXT_CLIP_BOTTOM, &promptHeight);

        cursorY += promptHeight + 10.0f;

        if (cursorY - 6.0f >= TEXT_Y && cursorY - 6.0f <= TEXT_CLIP_BOTTOM) {
            R_DrawHLine(TEXT_X + ROLE_W, cursorY - 6.0f, TEXT_W - ROLE_W, COLOR_BORDER);
        }
    }

    if (cursorY >= TEXT_Y && cursorY + 16.0f <= TEXT_CLIP_BOTTOM) {
        R_DrawText(TEXT_X, cursorY + 2.0f, 0.5f, "GEM", COLOR_TEXT_HIGHLIGHT);
    }

    if (responseText[0]) {
        R_DrawTextWrappedClipped(TEXT_X + ROLE_W, cursorY, TEXT_W - ROLE_W, 0.7f,
                                 responseText, COLOR_TEXT_NORMAL, 2.0f,
                                 TEXT_Y, TEXT_CLIP_BOTTOM, &responseHeight);
    } else {
        R_DrawText(TEXT_X + ROLE_W, cursorY, 0.7f, "NO RESPONSE", COLOR_TEXT_DIM);
        responseHeight = 0.0f;
    }

    totalTextHeight = promptHeight + (lastPrompt[0] ? 10.0f : 0.0f) + responseHeight;
    maxScrollY = totalTextHeight - TEXT_VIEW_H;
    if (maxScrollY < 0.0f) maxScrollY = 0.0f;
    if (scrollY > maxScrollY) scrollY = maxScrollY;

    R_DrawScrollbar(SCROLL_X, TEXT_Y, SCROLL_H, totalTextHeight, TEXT_VIEW_H, scrollY);
}

static void Draw_BottomScreen(CamMode cam_mode) {
    R_SetTarget(SCREEN_BOTTOM);
    R_ClearScreen(SCREEN_BOTTOM, COLOR_BACKGROUND);

    if (cam_mode == CAM_MODE_PREVIEW) {
        R_DrawButton(8.0f, CAM_PREVIEW_SHUTTER_Y, 304.0f, 56.0f,
                     "A", "SHUTTER", ICON_NONE, BTN_NORMAL);
        R_DrawButton(8.0f, CAM_PREVIEW_EXIT_Y, 304.0f, 56.0f,
                     "B", "EXIT", ICON_NONE, BTN_NORMAL);
        return;
    }

    if (cam_mode == CAM_MODE_CAPTURED) {
        R_DrawButton(8.0f, CAM_CAPTURED_USE_Y, 304.0f, CAM_BTN_H,
                     "A", "USE IMAGE", ICON_NONE, BTN_NORMAL);
        R_DrawButton(8.0f, CAM_CAPTURED_RETAKE_Y, 304.0f, CAM_BTN_H,
                     "X", "RETAKE", ICON_NONE, BTN_NORMAL);
        R_DrawButton(8.0f, CAM_CAPTURED_EXIT_Y, 304.0f, CAM_BTN_H,
                     "B", "EXIT", ICON_NONE, BTN_NORMAL);
        return;
    }

    if (lastPrompt[0]) {
        R_DrawTextEllipsized(14.0f, 24.0f, 292.0f, 0.55f,
                             lastPrompt, COLOR_TEXT_DIM);
    }

    R_DrawButtonLeft(BOT_BTN_X1, BOT_BTN_Y1, BOT_BTN_W, BOT_BTN_H,
                     "A", "ASK", BTN_NORMAL);
    R_DrawButtonLeft(BOT_BTN_X2, BOT_BTN_Y1, BOT_BTN_W, BOT_BTN_H,
                     "Y", "TALK", Mic_IsRecording() ? BTN_ACTIVE : BTN_NORMAL);
    R_DrawButtonLeft(BOT_BTN_X1, BOT_BTN_Y2, BOT_BTN_W, BOT_BTN_H,
                     "X", "CAMERA", BTN_NORMAL);
    R_DrawButtonLeft(BOT_BTN_X2, BOT_BTN_Y2, BOT_BTN_W, BOT_BTN_H,
                     "B", "BACK", BTN_NORMAL);
}

void GeminiApp_Draw() {
    if (Mic_IsRecording()) {
        Draw_RecordingScreen();
        return;
    }

    CamMode cam_mode = Cam_GetMode();
    Draw_TopScreen(cam_mode);
    Draw_BottomScreen(cam_mode);
}
