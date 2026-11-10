#ifndef RENDERER_H
#define RENDERER_H

#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_BOTTOM_HEIGHT 240

#define R_THEME_COUNT 5

typedef enum {
    COLOR_BACKGROUND = 0,
    COLOR_PANEL,
    COLOR_PANEL_ALT,
    COLOR_BORDER,
    COLOR_TEXT_NORMAL,
    COLOR_TEXT_DIM,
    COLOR_TEXT_HIGHLIGHT,
    COLOR_CURSOR,
    COLOR_ACCENT,
    COLOR_ALERT,
    COLOR_SCROLL_BAR,
    COLOR_SCROLL_TRACK,
    COLOR_SHADOW
} Color;

typedef enum {
    BTN_NORMAL = 0,
    BTN_SELECTED,
    BTN_ACTIVE
} ButtonState;

typedef enum {
    ICON_NONE = 0,
    ICON_SPARKLE,
    ICON_CHAT,
    ICON_MIC,
    ICON_CAMERA,
    ICON_BACK,
    ICON_KEY,
    ICON_GEAR,
    ICON_ARROW_LEFT,
    ICON_ARROW_RIGHT
} Icon;

typedef struct {
    const char *key;
    const char *label;
} UIHint;

typedef enum {
    SCREEN_TOP,
    SCREEN_BOTTOM
} TargetScreen;

void R_Init();
void R_Exit();

void R_BeginFrame();
void R_EndFrame();

void R_SetTarget(TargetScreen screen);
void R_ClearScreen(TargetScreen screen, Color color);

/* Theme handling ------------------------------------------------------- */
void R_SetTheme(int index);
int R_GetThemeIndex(void);
const char *R_GetThemeName(int index);
const char *R_GetCurrentThemeName(void);

/* Text ----------------------------------------------------------------- */
/* This function aims to remove every markdown or indenting symbol
 * from the given text */
void R_ClearText(char *text);

void R_DrawText(float x, float y, float scale, const char *text, Color color);
void R_DrawTextCentered(float x, float y, float width, float scale, const char *text, Color color);
void R_DrawTextRight(float right, float y, float scale, const char *text, Color color);
void R_DrawTextEllipsized(float x, float y, float maxWidth, float scale, const char *text, Color color);

void R_DrawTextWrapped(float x, float y, float widthLimit, const char *text, Color color, float *totalTextHeight);
void R_DrawTextWrappedEx(float x, float y, float widthLimit, float scale, const char *text,
                         Color color, float lineGap, float *totalTextHeight);
void R_DrawTextWrappedClipped(float x, float y, float widthLimit, float scale, const char *text,
                              Color color, float lineGap, float clipTop, float clipBottom,
                              float *totalTextHeight);

float R_TextWidth(const char *text, float scale);
float R_TextHeight(const char *text, float scale);

/* Shapes and UI pieces ------------------------------------------------- */
void R_DrawRectSolid(float x, float y, float z, float width, float height, Color color);
void R_DrawRectSolidA(float x, float y, float z, float width, float height, u32 color);
void R_DrawHLine(float x, float y, float width, Color color);
void R_DrawVLine(float x, float y, float height, Color color);

void R_DrawPanel(float x, float y, float width, float height, bool shadow);
void R_DrawPanelAccent(float x, float y, float width, float height);

void R_DrawHeader(const char *title, const char *rightText);
void R_DrawHeaderW(float width, const char *title, const char *rightText);
void R_DrawFooterHintsW(float width, const UIHint *hints, int count);
void R_DrawFooterTextW(float width, const char *leftText, const char *rightText);
void R_DrawFooterTextColorW(float width, const char *leftText, const char *rightText,
                            Color leftColor, Color rightColor);

void R_DrawKeycap(float x, float y, const char *key, bool inverted);
float R_DrawHint(float x, float y, const char *key, const char *label, Color color);
void R_DrawButton(float x, float y, float width, float height, const char *key,
                  const char *label, Icon icon, ButtonState state);
void R_DrawButtonLeft(float x, float y, float width, float height, const char *key,
                      const char *label, ButtonState state);

void R_DrawIcon(float x, float y, float size, Icon icon, Color color);
void R_DrawLed(float x, float y, Color color);
void R_DrawSparkle(float x, float y, float size, Color color);
void R_DrawScrollbar(float x, float y, float height, float contentHeight,
                     float viewHeight, float scroll);

/* Touch ---------------------------------------------------------------- */
bool R_UpdateTouch(touchPosition *touch, bool *down, bool *up);
bool R_TouchInRect(const touchPosition *touch, float x, float y, float width, float height);
void R_SuppressTouch(void);

/* Camera / system ------------------------------------------------------ */
void R_DrawCameraFeed(u16 *buffer);
bool R_OpenKeyboard(const char *hintText, char *outputBuffer, size_t maxLen);

#endif
