#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <citro2d.h>

/* -------------------------------------------------------------------------
 * Theme system
 * ---------------------------------------------------------------------- */

typedef struct {
    const char *name;
    u32 bg;
    u32 panel;
    u32 panelAlt;
    u32 border;
    u32 text;
    u32 dim;
    u32 hot;
    u32 accent;
    u32 alert;
    u32 scrollBar;
    u32 scrollTrack;
    u32 shadow;
} UITheme;

static const UITheme themes[R_THEME_COUNT] = {
    {
        "PHOSPHOR",
        0x0A0E0AFF, 0x101710FF, 0x162016FF, 0x2A4530FF,
        0xA8FFB8FF, 0x5E8F6AFF, 0xF0FFF2FF, 0xFFB000FF,
        0xFF5F56FF, 0x3E7A4CFF, 0x162016FF, 0x050805FF
    },
    {
        "AMBER",
        0x120D06FF, 0x1B1409FF, 0x241B0CFF, 0x4A3414FF,
        0xFFB000FF, 0x9A6A1EFF, 0xFFF0C8FF, 0xFFE29AFF,
        0xFF6B5AFF, 0x8A5D1AFF, 0x241B0CFF, 0x080502FF
    },
    {
        "PC-98",
        0x0B0B1AFF, 0x14142EFF, 0x1B1B3DFF, 0x35356AFF,
        0xD8D8F2FF, 0x7474A8FF, 0xFFFFFFFF, 0xFF6E9CFF,
        0xFF5F56FF, 0x4A4A8AFF, 0x1B1B3DFF, 0x05050CFF
    },
    {
        "GEMINI",
        0x0C0F1EFF, 0x141A30FF, 0x1B2340FF, 0x2E3A66FF,
        0xC9D2FFFF, 0x6B76A8FF, 0xFFFFFFFF, 0x7CE7FFFF,
        0xFF5F7AFF, 0x3E4C8AFF, 0x1B2340FF, 0x05060DFF
    },
    {
        "LCD LIGHT",
        0xD7E2B8FF, 0xC9D6A4FF, 0xBCCB93FF, 0x7D8F5AFF,
        0x22331BFF, 0x5E7245FF, 0x0E180BFF, 0x9A4A2AFF,
        0xB03020FF, 0x6B7F45FF, 0xBCCB93FF, 0xA8B88AFF
    }
};

static int currentTheme = 0;

static u32 col(u32 hex) {
    return C2D_Color32((hex >> 24) & 0xFF, (hex >> 16) & 0xFF,
                       (hex >> 8) & 0xFF, hex & 0xFF);
}

static u32 getColor(Color color) {
    const UITheme *t = &themes[currentTheme];

    switch (color) {
        case COLOR_BACKGROUND:      return col(t->bg);
        case COLOR_PANEL:           return col(t->panel);
        case COLOR_PANEL_ALT:       return col(t->panelAlt);
        case COLOR_BORDER:          return col(t->border);
        case COLOR_TEXT_NORMAL:     return col(t->text);
        case COLOR_TEXT_DIM:        return col(t->dim);
        case COLOR_TEXT_HIGHLIGHT:  return col(t->hot);
        case COLOR_CURSOR:          return col(t->text);
        case COLOR_ACCENT:          return col(t->accent);
        case COLOR_ALERT:           return col(t->alert);
        case COLOR_SCROLL_BAR:      return col(t->scrollBar);
        case COLOR_SCROLL_TRACK:    return col(t->scrollTrack);
        case COLOR_SHADOW:          return col(t->shadow);
        default:                    return col(t->hot);
    }
}

void R_SetTheme(int index) {
    if (index < 0 || index >= R_THEME_COUNT) index = 0;
    currentTheme = index;
}

int R_GetThemeIndex(void) {
    return currentTheme;
}

const char *R_GetThemeName(int index) {
    if (index < 0 || index >= R_THEME_COUNT) return themes[0].name;
    return themes[index].name;
}

const char *R_GetCurrentThemeName(void) {
    return themes[currentTheme].name;
}

/* -------------------------------------------------------------------------
 * Citro2d state
 * ---------------------------------------------------------------------- */

static C3D_RenderTarget *topTarget;
static C3D_RenderTarget *bottomTarget;
static C2D_Font sysFont;
static C2D_TextBuf staticTextBuf;

static C3D_Tex camTex;
static u16 *alignedBuffer = NULL;
static Tex3DS_SubTexture camSubtex;
static C2D_Image camImage;
static bool camTexInitialized = false;

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

    /* Long Gemini answers are drawn word by word. Give the text buffer
     * enough room so it does not silently run out on long responses. */
    staticTextBuf = C2D_TextBufNew(131072);

    R_SetTheme(0);
}

void R_Exit() {
    if (camTexInitialized) {
        C3D_TexDelete(&camTex);
        if (alignedBuffer) linearFree(alignedBuffer);
    }
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

/* -------------------------------------------------------------------------
 * Text
 * ---------------------------------------------------------------------- */

void R_ClearText(char *text) {
    if (!text) return;

    char *src = text;
    char *dst = text;

    while (*src) {
        char c = *src;

        /* Markdown symbols */
        if (c == '`' || c == '*' || c == '#' || c == '_' || c == '~' ||
            c == '>' || c == '[' || c == ']') {
            src++;
            continue;
        }

        /* Indentation symbols */
        if (c == '\n' || c == '\t' || c == '\r') {
            *dst = ' ';
            dst++;
            src++;
            continue;
        }

        *dst = c;
        dst++;
        src++;
    }
    *dst = '\0';
}

void R_DrawText(float x, float y, float scale, const char *text, Color color) {
    if (!text || !*text) return;
    if (scale <= 0.0f) scale = 1.0f;

    C2D_Text txt;
    C2D_TextFontParse(&txt, sysFont, staticTextBuf, text);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, scale, scale, getColor(color));
}

float R_TextWidth(const char *text, float scale) {
    if (!text || !*text) return 0.0f;
    if (scale <= 0.0f) scale = 1.0f;

    C2D_Text txt;
    float width = 0.0f;
    C2D_TextFontParse(&txt, sysFont, staticTextBuf, text);
    C2D_TextOptimize(&txt);
    C2D_TextGetDimensions(&txt, scale, scale, &width, NULL);
    return width;
}

float R_TextHeight(const char *text, float scale) {
    if (!text || !*text) return 0.0f;
    if (scale <= 0.0f) scale = 1.0f;

    C2D_Text txt;
    float height = 0.0f;
    C2D_TextFontParse(&txt, sysFont, staticTextBuf, text);
    C2D_TextOptimize(&txt);
    C2D_TextGetDimensions(&txt, scale, scale, NULL, &height);
    return height;
}

void R_DrawTextCentered(float x, float y, float width, float scale, const char *text, Color color) {
    if (!text || !*text) return;
    float textWidth = R_TextWidth(text, scale);
    R_DrawText(x + (width - textWidth) * 0.5f, y, scale, text, color);
}

void R_DrawTextRight(float right, float y, float scale, const char *text, Color color) {
    if (!text || !*text) return;
    R_DrawText(right - R_TextWidth(text, scale), y, scale, text, color);
}

void R_DrawTextEllipsized(float x, float y, float maxWidth, float scale, const char *text, Color color) {
    if (!text || !*text || maxWidth <= 0.0f) return;
    if (scale <= 0.0f) scale = 1.0f;

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", text);

    int len = (int)strlen(buf);
    if (R_TextWidth(buf, scale) <= maxWidth) {
        R_DrawText(x, y, scale, buf, color);
        return;
    }

    /* Find the longest prefix that still fits. */
    int lo = 0;
    int hi = len;
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        char saved = buf[mid];
        buf[mid] = '\0';
        float w = R_TextWidth(buf, scale);
        buf[mid] = saved;

        if (w <= maxWidth) lo = mid;
        else hi = mid - 1;
    }

    if (lo > 3) {
        buf[lo - 3] = '.';
        buf[lo - 2] = '.';
        buf[lo - 1] = '.';
        buf[lo] = '\0';
    } else {
        buf[lo] = '\0';
    }

    R_DrawText(x, y, scale, buf, color);
}

void R_DrawTextWrappedClipped(float x, float y, float widthLimit, float scale,
                              const char *text, Color color, float lineGap,
                              float clipTop, float clipBottom, float *totalTextHeight) {
    if (totalTextHeight) *totalTextHeight = 0.0f;
    if (!text || !*text || widthLimit <= 0.0f) return;
    if (scale <= 0.0f) scale = 1.0f;

    char *textCpy = strdup(text);
    if (!textCpy) return;

    C2D_Text space;
    float spaceWidth = 0.0f;
    C2D_TextFontParse(&space, sysFont, staticTextBuf, " ");
    C2D_TextOptimize(&space);
    C2D_TextGetDimensions(&space, scale, scale, &spaceWidth, NULL);

    float cursorX = x;
    float cursorY = y;
    float currentLineMaxHeight = 0.0f;

    char *parsePtr = textCpy;
    char *savePtr = NULL;
    char *token = NULL;

    while ((token = strtok_r(parsePtr, " \n\t\r", &savePtr))) {
        parsePtr = NULL;

        C2D_Text word;
        float wordWidth = 0.0f;
        float wordHeight = 0.0f;

        C2D_TextFontParse(&word, sysFont, staticTextBuf, token);
        C2D_TextOptimize(&word);
        C2D_TextGetDimensions(&word, scale, scale, &wordWidth, &wordHeight);

        if (cursorX > x && cursorX + wordWidth > x + widthLimit) {
            cursorX = x;
            cursorY += (currentLineMaxHeight > 0.0f ? currentLineMaxHeight : wordHeight) + lineGap;
            currentLineMaxHeight = 0.0f;
        }

        if (cursorY >= clipTop && cursorY + wordHeight <= clipBottom) {
            C2D_DrawText(&word, C2D_WithColor, cursorX, cursorY, 0.5f,
                         scale, scale, getColor(color));
        }

        cursorX += wordWidth + spaceWidth;
        if (wordHeight > currentLineMaxHeight) currentLineMaxHeight = wordHeight;
    }

    free(textCpy);

    if (totalTextHeight) {
        *totalTextHeight = (cursorY - y) + currentLineMaxHeight;
    }
}

void R_DrawTextWrappedEx(float x, float y, float widthLimit, float scale,
                         const char *text, Color color, float lineGap,
                         float *totalTextHeight) {
    R_DrawTextWrappedClipped(x, y, widthLimit, scale, text, color, lineGap,
                             -100000.0f, 100000.0f, totalTextHeight);
}

void R_DrawTextWrapped(float x, float y, float widthLimit, const char *text,
                       Color color, float *totalTextHeight) {
    R_DrawTextWrappedEx(x, y, widthLimit, 1.0f, text, color, 2.0f, totalTextHeight);
}

/* -------------------------------------------------------------------------
 * Shapes
 * ---------------------------------------------------------------------- */

void R_DrawRectSolid(float x, float y, float z, float width, float height, Color color) {
    if (width <= 0.0f || height <= 0.0f) return;
    C2D_DrawRectSolid(x, y, z, width, height, getColor(color));
}

void R_DrawRectSolidA(float x, float y, float z, float width, float height, u32 color) {
    if (width <= 0.0f || height <= 0.0f) return;
    C2D_DrawRectSolid(x, y, z, width, height, color);
}

void R_DrawHLine(float x, float y, float width, Color color) {
    R_DrawRectSolid(x, y, 0.5f, width, 1.0f, color);
}

void R_DrawVLine(float x, float y, float height, Color color) {
    R_DrawRectSolid(x, y, 0.5f, 1.0f, height, color);
}

void R_DrawPanel(float x, float y, float width, float height, bool shadow) {
    if (width <= 0.0f || height <= 0.0f) return;

    (void)shadow;

    R_DrawRectSolidA(x, y, 0.5f, width, height, getColor(COLOR_PANEL));

    R_DrawRectSolidA(x, y, 0.5f, width, 1.0f, getColor(COLOR_BORDER));
    R_DrawRectSolidA(x, y + height - 1.0f, 0.5f, width, 1.0f, getColor(COLOR_BORDER));
    R_DrawRectSolidA(x, y, 0.5f, 1.0f, height, getColor(COLOR_BORDER));
    R_DrawRectSolidA(x + width - 1.0f, y, 0.5f, 1.0f, height, getColor(COLOR_BORDER));
}

void R_DrawPanelAccent(float x, float y, float width, float height) {
    if (width <= 0.0f || height <= 0.0f) return;

    R_DrawRectSolidA(x, y, 0.5f, width, height, getColor(COLOR_PANEL_ALT));

    R_DrawRectSolidA(x, y, 0.5f, width, 1.0f, getColor(COLOR_BORDER));
    R_DrawRectSolidA(x, y + height - 1.0f, 0.5f, width, 1.0f, getColor(COLOR_BORDER));
    R_DrawRectSolidA(x + width - 1.0f, y, 0.5f, 1.0f, height, getColor(COLOR_BORDER));

    /* Single accent bar instead of a full glowing frame. */
    R_DrawRectSolidA(x, y, 0.5f, 3.0f, height, getColor(COLOR_ACCENT));
}

/* -------------------------------------------------------------------------
 * Composite UI pieces
 * ---------------------------------------------------------------------- */

void R_DrawHeaderW(float width, const char *title, const char *rightText) {
    R_DrawRectSolidA(0.0f, 0.0f, 0.5f, width, 24.0f, getColor(COLOR_PANEL));
    R_DrawRectSolidA(0.0f, 0.0f, 0.5f, 2.0f, 24.0f, getColor(COLOR_ACCENT));
    R_DrawHLine(0.0f, 23.0f, width, COLOR_BORDER);

    float rightWidth = (rightText && *rightText) ? R_TextWidth(rightText, 0.45f) : 0.0f;
    float titleMax = width - 14.0f - 10.0f - rightWidth - 10.0f;
    if (titleMax < 40.0f) titleMax = 40.0f;

    if (title) {
        R_DrawTextEllipsized(10.0f, 4.0f, titleMax, 0.6f, title, COLOR_TEXT_HIGHLIGHT);
    }

    if (rightText && *rightText) {
        R_DrawText(width - 10.0f - rightWidth, 7.0f, 0.45f, rightText, COLOR_TEXT_DIM);
    }
}

void R_DrawHeader(const char *title, const char *rightText) {
    R_DrawHeaderW(SCREEN_TOP_WIDTH, title, rightText);
}

void R_DrawKeycap(float x, float y, const char *key, bool inverted) {
    if (!key || !*key) return;

    u32 fill = inverted ? getColor(COLOR_ACCENT) : getColor(COLOR_PANEL_ALT);
    u32 border = inverted ? getColor(COLOR_ACCENT) : getColor(COLOR_BORDER);
    Color textColor = inverted ? COLOR_BACKGROUND : COLOR_TEXT_DIM;

    R_DrawRectSolidA(x, y, 0.5f, 13.0f, 13.0f, border);
    R_DrawRectSolidA(x + 1.0f, y + 1.0f, 0.5f, 11.0f, 11.0f, fill);

    float tw = R_TextWidth(key, 0.45f);
    float th = R_TextHeight(key, 0.45f);
    R_DrawText(x + (13.0f - tw) * 0.5f, y + (13.0f - th) * 0.5f,
               0.45f, key, textColor);
}

float R_DrawHint(float x, float y, const char *key, const char *label, Color color) {
    float labelWidth = 0.0f;

    if (key && *key) {
        R_DrawKeycap(x, y, key, false);
        x += 16.0f;
    }

    if (label && *label) {
        R_DrawText(x, y, 0.45f, label, color);
        labelWidth = R_TextWidth(label, 0.45f);
    }

    return (key && *key ? 16.0f : 0.0f) + labelWidth + 10.0f;
}

void R_DrawFooterHintsW(float width, const UIHint *hints, int count) {
    R_DrawHLine(0.0f, SCREEN_TOP_HEIGHT - 23.0f, width, COLOR_BORDER);

    float x = 10.0f;
    float y = SCREEN_TOP_HEIGHT - 18.0f;

    for (int i = 0; i < count; i++) {
        float hintWidth = (hints[i].key && *hints[i].key ? 16.0f : 0.0f) +
                          R_TextWidth(hints[i].label, 0.45f);
        if (x + hintWidth > width - 10.0f) break;

        x += R_DrawHint(x, y, hints[i].key, hints[i].label, COLOR_TEXT_DIM);
    }
}

void R_DrawFooterTextColorW(float width, const char *leftText, const char *rightText,
                            Color leftColor, Color rightColor) {
    R_DrawHLine(0.0f, SCREEN_TOP_HEIGHT - 23.0f, width, COLOR_BORDER);

    float rightWidth = (rightText && *rightText) ? R_TextWidth(rightText, 0.45f) : 0.0f;
    float leftMax = width - 20.0f - rightWidth - 10.0f;

    if (leftText && *leftText) {
        R_DrawTextEllipsized(10.0f, SCREEN_TOP_HEIGHT - 18.0f, leftMax, 0.45f,
                             leftText, leftColor);
    }

    if (rightText && *rightText) {
        R_DrawText(width - 10.0f - rightWidth, SCREEN_TOP_HEIGHT - 18.0f, 0.45f,
                   rightText, rightColor);
    }
}

void R_DrawFooterTextW(float width, const char *leftText, const char *rightText) {
    R_DrawFooterTextColorW(width, leftText, rightText, COLOR_TEXT_DIM, COLOR_TEXT_NORMAL);
}

static void DrawButtonText(float x, float y, float width, float height,
                           const char *key, const char *label, Icon icon,
                           ButtonState state, bool centered) {
    if (width <= 0.0f || height <= 0.0f) return;

    bool selected = (state == BTN_SELECTED);
    bool active = (state == BTN_ACTIVE);

    Color labelColor = selected ? COLOR_ACCENT
                                : (active ? COLOR_ALERT : COLOR_TEXT_NORMAL);
    Color iconColor = selected ? COLOR_ACCENT
                               : (active ? COLOR_ALERT : COLOR_TEXT_DIM);

    float labelScale = (height < 42.0f) ? 0.6f : 0.7f;
    float labelHeight = R_TextHeight(label, labelScale);
    float labelWidth = R_TextWidth(label, labelScale);

    float keyWidth = (key && *key) ? 13.0f : 0.0f;
    float keyGap = (key && *key) ? 8.0f : 0.0f;
    float iconWidth = (icon != ICON_NONE) ? 20.0f : 0.0f;
    float contentWidth = keyWidth + keyGap + labelWidth + iconWidth;

    float startX = centered ? (x + (width - contentWidth) * 0.5f) : (x + 12.0f);
    float textY = y + (height - labelHeight) * 0.5f;

    if (key && *key) {
        R_DrawKeycap(startX, y + (height - 13.0f) * 0.5f, key, selected);
    }

    float labelX = startX + keyWidth + keyGap;
    if (label && *label) {
        R_DrawText(labelX, textY, labelScale, label, labelColor);
    }

    if (icon != ICON_NONE) {
        R_DrawIcon(labelX + labelWidth + 8.0f, y + (height - 12.0f) * 0.5f,
                   12.0f, icon, iconColor);
    }

    if (selected || active) {
        R_DrawRectSolid(startX, textY + labelHeight + 3.0f, 0.5f,
                        contentWidth, 2.0f,
                        selected ? COLOR_ACCENT : COLOR_ALERT);
    }
}

void R_DrawButton(float x, float y, float width, float height, const char *key,
                  const char *label, Icon icon, ButtonState state) {
    DrawButtonText(x, y, width, height, key, label, icon, state, true);
}

void R_DrawButtonLeft(float x, float y, float width, float height, const char *key,
                      const char *label, ButtonState state) {
    DrawButtonText(x, y, width, height, key, label, ICON_NONE, state, false);
}

/* -------------------------------------------------------------------------
 * Pixel icons
 * ---------------------------------------------------------------------- */

void R_DrawIcon(float x, float y, float size, Icon icon, Color color) {
    if (size <= 0.0f || icon == ICON_NONE) return;

    u32 c = getColor(color);
    float u = size / 12.0f;

#define PX(gx, gy, gw, gh) \
    R_DrawRectSolidA(x + (gx) * u, y + (gy) * u, 0.5f, (gw) * u, (gh) * u, c)

    switch (icon) {
        case ICON_SPARKLE:
            PX(5, 0, 2, 12);
            PX(0, 5, 12, 2);
            PX(2, 2, 2, 2);
            PX(8, 2, 2, 2);
            PX(2, 8, 2, 2);
            PX(8, 8, 2, 2);
            break;

        case ICON_CHAT:
            PX(1, 1, 10, 7);
            PX(3, 8, 3, 2);
            break;

        case ICON_MIC:
            PX(4, 1, 4, 6);
            PX(5, 7, 2, 2);
            PX(3, 9, 6, 2);
            break;

        case ICON_CAMERA:
            PX(1, 3, 10, 2);
            PX(1, 8, 10, 2);
            PX(1, 3, 2, 7);
            PX(9, 3, 2, 7);
            PX(3, 1, 4, 2);
            PX(4, 5, 4, 3);
            break;

        case ICON_BACK:
            PX(1, 5, 10, 2);
            PX(1, 3, 2, 2);
            PX(1, 7, 2, 2);
            PX(3, 1, 2, 2);
            PX(3, 9, 2, 2);
            break;

        case ICON_KEY:
            PX(1, 2, 4, 1);
            PX(1, 5, 4, 1);
            PX(1, 2, 1, 4);
            PX(4, 2, 1, 4);
            PX(5, 4, 6, 2);
            PX(8, 6, 2, 2);
            PX(10, 6, 2, 2);
            break;

        case ICON_GEAR:
            PX(4, 4, 4, 4);
            PX(5, 1, 2, 2);
            PX(5, 9, 2, 2);
            PX(1, 5, 2, 2);
            PX(9, 5, 2, 2);
            PX(2, 2, 2, 2);
            PX(8, 2, 2, 2);
            PX(2, 8, 2, 2);
            PX(8, 8, 2, 2);
            break;

        case ICON_ARROW_LEFT:
            PX(2, 5, 9, 2);
            PX(1, 3, 2, 6);
            PX(3, 1, 2, 2);
            PX(3, 9, 2, 2);
            break;

        case ICON_ARROW_RIGHT:
            PX(1, 5, 9, 2);
            PX(9, 3, 2, 6);
            PX(7, 1, 2, 2);
            PX(7, 9, 2, 2);
            break;

        default:
            break;
    }

#undef PX
}

void R_DrawSparkle(float x, float y, float size, Color color) {
    R_DrawIcon(x, y, size, ICON_SPARKLE, color);
}

void R_DrawLed(float x, float y, Color color) {
    R_DrawRectSolidA(x, y, 0.5f, 6.0f, 6.0f, getColor(color));
}

void R_DrawScrollbar(float x, float y, float height, float contentHeight,
                     float viewHeight, float scroll) {
    if (contentHeight <= viewHeight || height <= 0.0f) return;

    R_DrawRectSolidA(x, y, 0.5f, 3.0f, height, getColor(COLOR_SCROLL_TRACK));

    float thumbHeight = height * (viewHeight / contentHeight);
    if (thumbHeight < 16.0f) thumbHeight = 16.0f;
    if (thumbHeight > height) thumbHeight = height;

    float maxScroll = contentHeight - viewHeight;
    if (maxScroll < 1.0f) maxScroll = 1.0f;

    float t = scroll / maxScroll;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float thumbY = y + t * (height - thumbHeight);
    R_DrawRectSolidA(x, thumbY, 0.5f, 3.0f, thumbHeight, getColor(COLOR_SCROLL_BAR));
}

/* -------------------------------------------------------------------------
 * Touch
 * ---------------------------------------------------------------------- */

static bool wasTouching = false;

bool R_UpdateTouch(touchPosition *touch, bool *down, bool *up) {
    if (touch) hidTouchRead(touch);

    bool touching = (hidKeysHeld() & KEY_TOUCH) != 0;

    if (down) *down = touching && !wasTouching;
    if (up) *up = !touching && wasTouching;

    wasTouching = touching;
    return touching;
}

bool R_TouchInRect(const touchPosition *touch, float x, float y, float width, float height) {
    if (!touch) return false;

    float px = (float)touch->px;
    float py = (float)touch->py;

    return px >= x && px < x + width && py >= y && py < y + height;
}

void R_SuppressTouch(void) {
    /* Blocking operations (keyboard, network) leave the cached touch state
     * stale.  Treat the next frame as still touched so a finger held across
     * the operation cannot trigger a fresh button press on return. */
    wasTouching = true;
}

/* -------------------------------------------------------------------------
 * Camera feed
 * ---------------------------------------------------------------------- */

void R_DrawCameraFeed(u16 *buffer) {
    if (!buffer) return;

    if (!camTexInitialized) {
        C3D_TexInit(&camTex, 512, 256, GPU_RGB565);
        alignedBuffer = (u16*)linearAlloc(512 * 256 * sizeof(u16));
        memset(alignedBuffer, 0, 512 * 256 * sizeof(u16));

        camSubtex.width = 400;
        camSubtex.height = 240;
        camSubtex.left = 0.0f;
        camSubtex.top = 240.0f / 256.0f;
        camSubtex.right = 400.0f / 512.0f;
        camSubtex.bottom = 16.0f / 256.0f;

        camImage.tex = &camTex;
        camImage.subtex = &camSubtex;

        camTexInitialized = true;
    }

    GSPGPU_InvalidateDataCache(buffer, 400 * 240 * sizeof(u16));

    for (int y = 0; y < 240; y++) {
        memcpy(&alignedBuffer[y * 512], &buffer[y * 400], 400 * sizeof(u16));
    }

    GSPGPU_FlushDataCache(alignedBuffer, 512 * 256 * sizeof(u16));

    C3D_SyncDisplayTransfer(
        (u32*)alignedBuffer, GX_BUFFER_DIM(512, 256),
        (u32*)camTex.data, GX_BUFFER_DIM(512, 256),
        (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
         GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) |
         GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) |
         GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
    );

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, 0xFFFFFFFF, 0.0f);

    C2D_DrawParams params = {
        .pos = { .x = 0.0f, .y = 0.0f, .w = 400.0f, .h = 240.0f },
        .center = { .x = 0.0f, .y = 0.0f },
        .angle = 0.0f,
        .depth = 0.5f
    };

    C2D_DrawImage(camImage, &params, &tint);
}

/* -------------------------------------------------------------------------
 * System keyboard
 * ---------------------------------------------------------------------- */

bool R_OpenKeyboard(const char *hintText, char *outputBuffer, size_t maxLen) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);

    swkbdSetInitialText(&swkbd, outputBuffer);
    swkbdSetHintText(&swkbd, hintText);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Confirm", true);

    SwkbdButton button = swkbdInputText(&swkbd, outputBuffer, maxLen);

    R_SuppressTouch();
    return (button == SWKBD_BUTTON_RIGHT);
}
