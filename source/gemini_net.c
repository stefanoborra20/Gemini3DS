#include "gemini_net.h"
#include <3ds.h>
#include <stdlib.h>
#include <malloc.h>

#define SOC_ALIGN 0x1000
#define SOC_BUFFER_SIZE 0x100000

static u32* socBuffer = NULL;

void Net_Init() {
    socBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFER_SIZE);
    if (socBuffer) socInit(socBuffer, SOC_BUFFER_SIZE);
}

void Net_Exit() {
    socExit();
    if (socBuffer) free(socBuffer);
}
