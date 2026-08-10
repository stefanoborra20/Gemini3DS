#include "gemini_net.h"
#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <curl/curl.h>
#include <jansson.h>
#include "api_key_manager.h"

#define SOC_ALIGN 0x1000
#define SOC_BUFFER_SIZE 0x100000

static u32* socBuffer = NULL;
static NetStatusCallback g_statusCallback = NULL;

void Net_Init() {
    socBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFER_SIZE);
    if (socBuffer) socInit(socBuffer, SOC_BUFFER_SIZE);
}

void Net_Exit() {
    socExit();
    if (socBuffer) free(socBuffer);
}

typedef struct {
    char *memory;
    size_t size;
    size_t maxSize;
} ResponseData;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realSize = size * nmemb;
    ResponseData *mem = (ResponseData *)userp;

    char *ptr = realloc(mem->memory, mem->size + realSize + 1);
    if (!ptr) return 0;
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realSize);
    mem->size += realSize;
    mem->memory[mem->size] = '\0';
    
    return realSize;
}

static char *Base64_Encode(const unsigned char *data, size_t length) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_length = 4 * ((length+2) / 3);
    char *encoded_data = malloc(output_length + 1);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < length;) {
        uint32_t octet_a = i < length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < (3 - length % 3) % 3; i++) {
        encoded_data[output_length - 1 - i] = '=';
    }

    encoded_data[output_length] = '\0';
    return encoded_data;
}

static char* Create_JSON(const char *prompt, u8 *mediaData, u32 mediaSize, const char *mimeType) {
    json_t *root = json_object();
    json_t *contents = json_array();
    json_t *content_obj = json_object();
    json_t *parts = json_array();

    // 1. Add text prompt
    json_t *text_part = json_object();
    json_object_set_new(text_part, "text", json_string(prompt ? prompt : ""));
    json_array_append_new(parts, text_part);

    // 2. Add media (Audio or Image) if provided
    if (mediaData && mediaSize > 0 && mimeType) {
        char *base64Str = Base64_Encode(mediaData, mediaSize);
        if (base64Str) {
            json_t *media_part = json_object();
            json_t *inline_data = json_object();

            json_object_set_new(inline_data, "mimeType", json_string(mimeType));
            json_object_set_new(inline_data, "data", json_string(base64Str));
            json_object_set_new(media_part, "inlineData", inline_data);

            json_array_append_new(parts, media_part);

            free(base64Str);
        } else {
            json_decref(root);
            return NULL;
        }
    }

    json_object_set_new(content_obj, "parts", parts);
    json_array_append_new(contents, content_obj);
    json_object_set_new(root, "contents", contents);

    char *json_string = json_dumps(root, 0);
    json_decref(root);
    return json_string;
}

void Net_SetStatusCallback(NetStatusCallback cb) {
    g_statusCallback = cb;
}

static char* Perform_CURL_Request(const char *jsonBody, char *outBuffer, size_t outBufSize) {
    int max_tries = ApiManager_IsRotationEnabled() ? ApiManager_GetTotalKeys() : 1;
    if (max_tries == 0) max_tries = 1;

    int current_try = 0;
    ResponseData chunk;
    chunk.memory = NULL;

    while (current_try < max_tries) {
        const char *apiKey = ApiManager_GetActiveKey();
        if (!apiKey || strlen(apiKey) == 0) {
            snprintf(outBuffer, outBufSize, "Error: No API Key available.");
            return NULL;
        }

        CURL *curl = curl_easy_init();
        if (!curl) return NULL;

        char url[512];
        snprintf(url, sizeof(url),
                 "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
                 Settings_GetModel(), apiKey);

        chunk.memory = malloc(1); 
        chunk.size = 0;
        if (chunk.memory) chunk.memory[0] = '\0';

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &chunk);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            snprintf(outBuffer, outBufSize, "Curl Error: %s", curl_easy_strerror(res));
            free(chunk.memory);
            return NULL;
        } 
        
        // --- AUTO ROTATION LOGIC ---
        if (http_code == 429 || http_code == 503) {
            free(chunk.memory);

            if (ApiManager_IsRotationEnabled() && current_try < (max_tries - 1)) {
                ApiManager_RotateKey();
                current_try++;
               
                if (g_statusCallback) {
                    char status[64];
                    snprintf(status, sizeof(status), "://Rate Limit. Trying key %d...", ApiManager_GetActiveKeyIndex()+1);
                    g_statusCallback(status);
                }
                continue;

            } else {
                snprintf(outBuffer, outBufSize, "HTTP Error %ld: Rate Limit/Overloaded", http_code);
                return NULL;
            }
        } 
        else if (http_code != 200) {
            snprintf(outBuffer, outBufSize, "HTTP Error %ld", http_code);
            free(chunk.memory);
            return NULL;
        }
        return chunk.memory; 
    }

    snprintf(outBuffer, outBufSize, "Failed after multiple key retries.");
    return NULL;
}

static bool Parse_Gemini_Response(const char *jsonString, char *outBuffer, size_t bufferSize) {
    json_error_t error;
    json_t *root = json_loads(jsonString, 0, &error);

    if (!root) {
        snprintf(outBuffer, bufferSize, "JSON Parse Error: %s", error.text);
        return false;
    }

    bool success = false;
    json_t *candidates = json_object_get(root, "candidates");

    if (json_array_size(candidates) > 0) {
        json_t *cand0 = json_array_get(candidates, 0);
        json_t *content = json_object_get(cand0, "content");
        json_t *parts = json_object_get(content, "parts");
        json_t *part0 = json_array_get(parts, 0);
        json_t *text_obj = json_object_get(part0, "text");

        const char *text_content = json_string_value(text_obj);

        if (text_content) {
            strncpy(outBuffer, text_content, bufferSize - 1);
            outBuffer[bufferSize - 1] = '\0';
            success = true;
        }
    } else {
        snprintf(outBuffer, bufferSize, "API ERR: No content returned.");
    }

    json_decref(root);
    return success;
}

bool Net_QueryGemini(const char *prompt, char *responseBuffer, size_t bufferSize) {
    char *jsonBody = Create_JSON(prompt, NULL, 0, NULL);
    if (!jsonBody) return false;

    char *rawJson = Perform_CURL_Request(jsonBody, responseBuffer, bufferSize);
    free(jsonBody);

    if (rawJson) {
        bool success = Parse_Gemini_Response(rawJson, responseBuffer, bufferSize);
        free(rawJson); 
        return success;
    }
    return false;
}

bool Net_QueryGeminiAudio(const char *prompt, u8 *audioData, u32 audioSize, char *responseBuffer, size_t bufferSize) {
    char *jsonBody = Create_JSON(prompt, audioData, audioSize, "audio/wav");
    if (!jsonBody) return false;

    char *rawJson = Perform_CURL_Request(jsonBody, responseBuffer, bufferSize);
    free(jsonBody);

    if (rawJson) {
        bool success = Parse_Gemini_Response(rawJson, responseBuffer, bufferSize);
        free(rawJson);
        return success;
    }
    return false;
}

bool Net_QueryGeminiImage(const char *prompt, u8 *imageData, size_t imageSize, char *responseBuffer, size_t bufferSize) {
    char *jsonBody = Create_JSON(prompt, imageData, imageSize, "image/jpeg");
    if (!jsonBody) return false;

    char *rawJson = Perform_CURL_Request(jsonBody, responseBuffer, bufferSize);
    free(jsonBody);

    if (rawJson) {
        bool success = Parse_Gemini_Response(rawJson, responseBuffer, bufferSize);
        free(rawJson);
        return success;
    }
    return false;
}

