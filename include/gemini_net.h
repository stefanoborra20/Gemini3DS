#ifndef GEMINI_NET_H
#define GEMINI_NET_H

#include <stdbool.h>
#include <stddef.h>
#include "settings.h"

void Net_Init();

void Net_Exit();

bool Net_QueryGemini(const char *apiKey, const char *promt, char *responseBuffer, size_t bufferSize);

bool Net_QueryGeminiAudio(const char *apiKey, const char *promt, u8 *audioData, u32 audioSize, char *responseBuffer, size_t bufferSize);

#endif
