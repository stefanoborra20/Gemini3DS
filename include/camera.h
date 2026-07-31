#ifndef CAMERA_H
#define CAMERA_H

#include <3ds.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    unsigned char **data;
    size_t size;
    size_t capacity;
} JpegImage;

typedef enum {
    CAM_MODE_OFF,
    CAM_MODE_PREVIEW,
    CAM_MODE_CAPTURED
} CamMode;

typedef struct {
    u16 width, height;
    bool initialized;
    CamMode mode;
} CamState;

bool Cam_Init();
void Cam_Exit();

bool Cam_StartPreview();
void Cam_UpdatePreview();
void Cam_StopPreview();

void Cam_FreezeFrame();
void Cam_ResumePreview();
void Cam_DrawTopScreen();

u16* Cam_GetBuffer();
CamMode Cam_GetMode();

bool Cam_Capture();

#endif
