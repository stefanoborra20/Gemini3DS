#ifndef MENU_H
#define MENU_H

#include <3ds.h>

typedef enum {
    MENU_ACTION_NONE,
    MENU_ACTION_GOTO_GEMINI,
    MENU_ACTION_GOTO_APIKEY
} MenuAction;

void Menu_Init();

MenuAction Menu_Update(u32 kDown);

void Menu_Draw();

#endif
