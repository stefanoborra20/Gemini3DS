#ifndef MIC_SYSTEM_H
#define MIC_SYSTEM_H

#include <3ds.h>

typedef struct {
    char riff[4];
    u32 size;
    char wave[4];
    char fmt_chunk_marker[4];
    u32 fmt_length;
    u16 format_type;
    u16 channels;
    u32 sample_rate;
    u32 byterate;
    u16 block_align;
    u16 bits_per_sample;
    char data_chunk_header[4];
    u32 data_size;
} __attribute__((packed)) WavHeader;

void Mic_Init();
void Mic_Exit();

void Mic_StartRecording();
void Mic_StopRecording();
void Mic_Update();

bool Mic_IsRecording();
u8* Mic_GetWavBuffer(); // Returns header + data
u32 Mic_GetWavSize();

#endif
