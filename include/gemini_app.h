#ifndef GEMINI_APP_H
#define GEMINI_APP_H

#include <3ds.h>

void GeminiApp_Init();

void GeminiApp_Update(u32 kDown, const char *apiKey);

void GeminiApp_Draw();

#endif
