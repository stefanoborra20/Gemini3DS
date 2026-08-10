#ifndef GEMINI_NET_H
#define GEMINI_NET_H

#include <stdbool.h>
#include <stddef.h>
#include "settings.h"

typedef void (*NetStatusCallback)(const char *statusMsg);

void Net_Init();

void Net_Exit();

bool Net_QueryGemini(const char *prompt, char *responseBuffer, size_t bufferSize);

bool Net_QueryGeminiAudio(const char *prompt, u8 *audioData, u32 audioSize, char *responseBuffer, size_t bufferSize);

bool Net_QueryGeminiImage(const char *prompt, u8 *imageData, size_t imageSize, char *responseBuffer, size_t bufferSize);

void Net_SetStatusCallback(NetStatusCallback cb);

#endif
