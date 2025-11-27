#ifndef MEM_H
#define MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define API_KEY_MAX_LEN 128

void Mem_SaveApiKey(const char *key);

bool Mem_LoadApiKey(char *buffer, size_t bufferSize);

#endif
