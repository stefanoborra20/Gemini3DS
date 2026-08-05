#ifndef GEMINI_NET_H
#define GEMINI_NET_H

#include <stdbool.h>
#include <stddef.h>
#include "settings.h"

void Net_Init();

void Net_Exit();

bool Net_QueryGemini(const char *apiKey, const char *prompt, char *responseBuffer, size_t bufferSize);

bool Net_QueryGeminiAudio(const char *apiKey, const char *prompt, u8 *audioData, u32 audioSize, char *responseBuffer, size_t bufferSize);

bool Net_QueryGeminiImage(const char *apiKey, const char *prompt, u8 *imageData, size_t imageSize, char *responseBuffer, size_t bufferSize);

#endif
