#include "renderer.h"
#include <citro2d.h>

static C3D_RenderTarget *topTarget;
static C3D_RenderTarget *bottomTarget;
static C2D_Font sysFont;
static C2D_TextBuf staticTextBuf;

static u32 getColor(Color color) {
    switch (color) {
        case COLOR_BACKGROUND: return C2D_Color32(0x30, 0x30, 0x30, 0xFF);
        case COLOR_TEXT_NORMAL: return C2D_Color32(0x33, 0xFF, 0x33, 0xFF); // green
        case COLOR_TEXT_HIGHLIGHT: return C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
        case COLOR_CURSOR: return C2D_Color32(0x33, 0xFF, 0x33, 0xFF);
        default: return C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
    }
}

void R_Init() {
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        printf("romfsInit() Failed\n");
    }

    gfxInitDefault();
    cfguInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    sysFont = C2D_FontLoad("romfs:/boldpixels.bcfnt");
    if (sysFont == NULL) {
        sysFont = C2D_FontLoadSystem(CFG_REGION_EUR);
    }
    staticTextBuf = C2D_TextBufNew(4096);
}

void R_Exit() {
    C2D_TextBufDelete(staticTextBuf);
    C2D_FontFree(sysFont);
    romfsExit();
    cfguExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void R_BeginFrame() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    // Clear Text Buffer
    C2D_TextBufClear(staticTextBuf);
}

void R_EndFrame() {
    C3D_FrameEnd(0);
}

void R_SetTarget(TargetScreen screen) {
    if (screen == SCREEN_TOP) C2D_SceneBegin(topTarget);
    else C2D_SceneBegin(bottomTarget);
}

void R_ClearScreen(TargetScreen screen, Color color) {
    if (screen == SCREEN_TOP) C2D_TargetClear(topTarget, getColor(color));
    else C2D_TargetClear(bottomTarget, getColor(color));
}

void R_DrawText(float x, float y, const char *text, Color color) {
    C2D_Text txt;
//    C2D_TextParse(&txt, staticTextBuf, text);
    C2D_TextFontParse(&txt, sysFont, staticTextBuf, text);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, 1.0f, 1.0f, getColor(color));
}

bool R_OpenKeyboard(const char *hintText, char *outputBuffer, size_t maxLen) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);

    // Keyboard Settings
    swkbdSetInitialText(&swkbd, outputBuffer);
    swkbdSetHintText(&swkbd, hintText);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Confirm", true);

    SwkbdButton button = swkbdInputText(&swkbd, outputBuffer, maxLen);

    return (button == SWKBD_BUTTON_RIGHT);
}
