#include "settings.h"

#define DEFAULT_MODEL MODEL_2_5_FLASH
#define DEFAULT_TEMPERATURE 1.0f
#define DEFAULT_MAX_TOKENS 1000

static AppSettings settings;

void Settings_Init() {
    settings.model = DEFAULT_MODEL;
    settings.temperature = DEFAULT_TEMPERATURE;
    settings.maxTokens = DEFAULT_MAX_TOKENS;
}

const AppSettings* Setting_Get() {
    return &settings;
}

const char *Settings_GetModelToString(GeminiModel model) {
    const char *modelNames[] = {
        "gemini-2.5-flash",
        "gemini-3.0-pro"
    };

    if (model > MODEL_COUNT) return "unknown";

    return modelNames[model];
}

void Settings_CycleModel() {
    settings.model = (settings.model + 1) % MODEL_COUNT;
}

void Settings_CycleTemperature() {
    settings.temperature++;
    if (settings.temperature > 2.0f)
        settings.temperature = 0.0f;
}

void Settings_CycleMaxTokens() {
    settings.maxTokens++;
    if (settings.maxTokens > 1000)
        settings.maxTokens = 100;
    
}
