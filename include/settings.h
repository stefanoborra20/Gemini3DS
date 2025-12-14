#ifndef SETTINGS_H
#define SETTINGS_H



typedef enum {
    MODEL_2_5_FLASH,
    MODEL_3_PRO,
    MODEL_COUNT
} GeminiModel;

typedef struct {
    GeminiModel model;
    float temperature; // 1.0 - 2.0
    int maxTokens; // 100 - 1000 
} AppSettings;

void Settings_Init();

const AppSettings* Settings_Get(); 

const char* Settings_GetModelToString(GeminiModel model);

void Settings_CycleModel();

void Settings_CycleTemperature();

void Settings_CycleMaxTokens();


#endif
