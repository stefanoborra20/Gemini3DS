#ifndef GEMINI_APP_H
#define GEMINI_APP_H

#include <3ds.h>

void GeminiApp_Init();
void GeminiApp_Exit();

void GeminiApp_Update(u32 kDown);

void GeminiApp_Draw();

void ForceDrawStatus(const char *message);

#endif
