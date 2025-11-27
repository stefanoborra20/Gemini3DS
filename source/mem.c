#include "mem.h"
#include <stdio.h>

static const char *FILENAME = "gemini_api_key.txt";

void Mem_SaveApiKey(const char *key) {
    FILE *f = fopen(FILENAME, "w");
    if (f) {
        fprintf(f, "%s", key);
        fclose (f);
    }
}

bool Mem_LoadApiKey(char *buffer, size_t bufferSize) {
    FILE *f = fopen(FILENAME, "r");
    if (!f) return false;

    if (fgets(buffer, bufferSize, f) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        fclose(f);
        return true;
    }
    fclose(f);
    return false;
}
