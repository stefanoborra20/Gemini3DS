#include "mic_system.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define SAMPLE_RATE 16360
#define MIC_BUFFER_SIZE 0x30000
//#define MAX_AUDIO_SIZE (1024 * 1024)
#define MAX_AUDIO_SIZE (384*1024)

static u8 *micBuffer = NULL;
static u8 *audioBuffer = NULL;

static bool isRecording = false;
static u32 audioWritePos = 0; // Where we are writing in the final buffer
static u32 micReadPos = 0;  // Where we are reading from the mic buf
static u32 micDataSize = 0;

void Mic_Init() {
    micBuffer = memalign(0x1000, MIC_BUFFER_SIZE);
    audioBuffer = malloc(MAX_AUDIO_SIZE);

    if (micBuffer) {
        micInit(micBuffer, MIC_BUFFER_SIZE);
        micDataSize = micGetSampleDataSize();
    }

    // Leave space for header
    audioWritePos = sizeof(WavHeader);
}

void Mic_Exit() {
    micExit();
    if (micBuffer) free(micBuffer);
    if (audioBuffer) free(audioBuffer);
}

void Mic_StartRecording() {
    if (!micBuffer || isRecording) return;

    audioWritePos = sizeof(WavHeader);
    micReadPos = 0;

    MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, micDataSize, true);
    isRecording = true;
}

static void InitWavHeader() {
    if (!audioBuffer) return;

    u32 rawAudioSize = audioWritePos - sizeof(WavHeader);
    WavHeader *header = (WavHeader*)audioBuffer;

    memcpy(header->riff, "RIFF", 4);
    header->size = rawAudioSize + sizeof(WavHeader) - 8;
    memcpy(header->wave, "WAVE", 4);
    memcpy(header->fmt_chunk_marker, "fmt ", 4);
    header->fmt_length = 16;
    header->format_type = 1;
    header->channels = 1;
    header->sample_rate = SAMPLE_RATE;
    header->bits_per_sample = 16;
    header->byterate = SAMPLE_RATE * 16 * 1 / 8;
    header->block_align = 2;
    memcpy(header->data_chunk_header, "data", 4);
    header->data_size = rawAudioSize;
}

void Mic_StopRecording() {
    if (!isRecording) return;

    MICU_StopSampling();

    isRecording = false;
    InitWavHeader();
}

void Mic_Update() {
    if (!isRecording) return;

    if (audioWritePos >= MAX_AUDIO_SIZE) {
        Mic_StopRecording();
        return;
    }

    u32 micCurrentHardwarePos = micGetLastSampleOffset();

    while (micReadPos != micCurrentHardwarePos) {
        audioBuffer[audioWritePos] = micBuffer[micReadPos];
        audioWritePos++;
        
        micReadPos = (micReadPos + 1) % micDataSize;

        if (audioWritePos >= MAX_AUDIO_SIZE) break;
    }
}

bool Mic_IsRecording() { return isRecording; }
u8* Mic_GetWavBuffer() { return audioBuffer; }
u32 Mic_GetWavSize() { return audioWritePos; }
