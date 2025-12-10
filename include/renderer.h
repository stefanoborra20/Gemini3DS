#ifndef RENDERER_H
#define RENDERER_H

#include <3ds.h>
#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOTTOM_WIDTH 340

typedef enum {
    COLOR_BACKGROUND=0,
    COLOR_TEXT_NORMAL,
    COLOR_TEXT_HIGHLIGHT,
    COLOR_CURSOR,
    COLOR_SCROLL_BAR
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

/* This function aims to remove every markdown or indenting symbol
 * from the given text */
void R_ClearText(char *text);

void R_DrawText(float x, float y, float scale, const char *text, Color color);
void R_DrawTextWrapped(float x, float y, float widthLimit, const char *text, Color color, float *totalTextHeight);
void R_DrawRectSolid(float x, float y, float z, float width, float height, Color color);

bool R_OpenKeyboard(const char *hintText, char *outputBuffer, size_t maxLen);

#endif
