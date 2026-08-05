#include "camera.h"
#include <stdlib.h>

CamState cam= {
    .width = 400, 
    .height = 240,
    .initialized = false,
    .mode = CAM_MODE_OFF
};

static u16* buffer = NULL;
static u32 captureSize = 0;
static u32 transferBytes = 0;

bool Cam_Init() {
    if (cam.initialized)
        return true;

    Result res = camInit();
    if (R_FAILED(res)) return false;

    captureSize = cam.width * cam.height * sizeof(u16);
    buffer = (u16*)linearAlloc(captureSize);
    if (!buffer) {
        camExit();
        return false;
    }

    cam.initialized = true;
    cam.mode = CAM_MODE_OFF;
    return true;
}

bool Cam_StartPreview() {
    if (!cam.initialized)
        if (!Cam_Init()) return false;

    CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);

    CAMU_GetMaxBytes(&transferBytes, cam.width, cam.height);
    CAMU_SetTransferBytes(PORT_CAM1, transferBytes, cam.width, cam.height);

    CAMU_Activate(SELECT_OUT1);
    CAMU_ClearBuffer(PORT_CAM1);

    cam.mode = CAM_MODE_PREVIEW;
    return true;
}

void Cam_UpdatePreview() {
    if (cam.mode != CAM_MODE_PREVIEW || !buffer) return;

    Handle camReceiveEvent = 0;
    
    CAMU_SetReceiving(&camReceiveEvent, buffer, PORT_CAM1, captureSize, (s16)transferBytes);
    
    CAMU_StartCapture(PORT_CAM1);
    
    Result res = svcWaitSynchronization(camReceiveEvent, 1000000000LL);
    
    CAMU_StopCapture(PORT_CAM1);
    
    svcCloseHandle(camReceiveEvent);

    if (R_FAILED(res)) {
        Cam_StopPreview();
    }
}

void Cam_FreezeFrame() {
    if (cam.mode == CAM_MODE_PREVIEW) {
        cam.mode = CAM_MODE_CAPTURED;
    }
}

void Cam_ResumePreview() {
    if (cam.mode == CAM_MODE_CAPTURED) {
        CAMU_ClearBuffer(PORT_CAM1); 
        cam.mode = CAM_MODE_PREVIEW;
    }
}

void Cam_StopPreview() {
    if (cam.mode != CAM_MODE_OFF) {
        CAMU_StopCapture(PORT_CAM1);
        CAMU_ClearBuffer(PORT_CAM1);
        CAMU_Activate(SELECT_NONE);
        cam.mode = CAM_MODE_OFF;
    }
}

void Cam_Exit() {
    Cam_StopPreview();
    if (buffer) {
        linearFree(buffer);
        buffer = NULL;
    }
    if (cam.initialized) {
        camExit();
        cam.initialized = false;
    }
}

u16* Cam_GetBuffer() {
    return buffer;
}

CamMode Cam_GetMode() {
    return cam.mode;
}

