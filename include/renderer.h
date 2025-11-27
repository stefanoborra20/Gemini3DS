#ifndef RENDERER_H
#define RENDERER_H

#include <3ds.h>

typedef enum {
    COLOR_BACKGROUND=0,
    COLOR_TEXT_NORMAL,
    COLOR_TEXT_HIGHLIGHT,
    COLOR_CURSOR
} Color;

typedef enum {
    SCREEN_TOP,
    SCREEN_BOTTOM
} TargetScreen;

void R_Init();
void R_Exit();

void R_BeginFrame();
void R_EndFrame();

void R_SetTarget(TargetScreen);

void R_ClearScreen(TargetScreen, Color);
void R_DrawText(float x, float y, const char *text, Color color);

bool R_OpenKeyboard(const char *hintText, char *outputBuffer, size_t maxLen);

#endif
