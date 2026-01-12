#include "gemini_net.h"
#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <curl/curl.h>
#include <jansson.h>

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

typedef struct {
    char *memory;
    size_t size;
    size_t maxSize;
} ResponseData;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realSize = size * nmemb;
    ResponseData *mem = (ResponseData *)userp;

    if (mem->size + realSize >= mem->maxSize) {
        if (mem->size < mem->maxSize - 1) {
            size_t safeSize = mem->maxSize - mem->size -1;
            memcpy(&(mem->memory[mem->size]), contents, safeSize);
            mem->size += safeSize;
        }
        mem->memory[mem->size] = '\0';
        return realSize;
    }

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

static char* Create_JSON(const char *promt, u8 *audioData, u32 audioSize) {
    json_t *root = json_object();
    json_t *contents = json_array();
    json_t *content_obj = json_object();
    json_t *parts = json_array();

    json_t *text_part = json_object();
    json_object_set_new(text_part, "text", json_string(promt ? promt : ""));
    json_array_append_new(parts, text_part);

    if (audioData && audioSize > 0) {
        char *base64Str = Base64_Encode(audioData, audioSize);
        if (base64Str) {
            json_t *audio_part = json_object();
            json_t *inline_data = json_object();

            json_object_set_new(inline_data, "mimeType", json_string("audio/wav"));
            json_object_set_new(inline_data, "data", json_string(base64Str));
            json_object_set_new(audio_part, "inlineData", inline_data);

            json_array_append_new(parts, audio_part);

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

static bool Perform_CURL_Request(const char *apiKey, const char *jsonBody, char *outBuffer, size_t outBufSize) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    bool success = false;
    char url[512];

    snprintf(url, sizeof(url),
           "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
           Settings_GetModel(),
           apiKey);

    ResponseData chunk;
    chunk.memory = outBuffer;
    chunk.size = 0;
    chunk.maxSize = outBufSize;
    outBuffer[0] = '\0';

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // CURL setup
    // Force IPv4 and longer timers
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &chunk);

    // Disable SSL
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        snprintf(outBuffer, outBufSize, "Curl Error: %s", curl_easy_strerror(res));
    } else if (http_code != 200) {
        char tmpErr[128];
        snprintf(tmpErr, sizeof(tmpErr), "HTTP Error %ld", http_code);
    } else {
        success = true;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return success;
}

static bool Parse_Gemini_Response(char *jsonBuffer, size_t bufferSize) {
    json_error_t error;
    json_t *root = json_loads(jsonBuffer, 0, &error);

    if (!root) {
        snprintf(jsonBuffer, bufferSize, "JSON Parse Error: %s", error.text);
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
            char *tmp = malloc(bufferSize);
            if (tmp) {
                strncpy(tmp, text_content, bufferSize - 1);
                tmp[bufferSize - 1] = '\0';

                strncpy(jsonBuffer, tmp, bufferSize - 1);
                jsonBuffer[bufferSize - 1] = '\0';
                free(tmp);

                success = true;
            }
        }
    } else {
        snprintf(jsonBuffer, bufferSize, "API ERR: Non content returned.");
    }

    json_decref(root);
    return success;
}

bool Net_QueryGemini(const char *apiKey, const char *promt, char *responseBuffer, size_t bufferSize) {
    char *jsonBody = Create_JSON(promt, NULL, 0);
    if (!jsonBody) return false;

    bool netSuccess = Perform_CURL_Request(apiKey, jsonBody, responseBuffer, bufferSize);
    free(jsonBody);

    if (netSuccess) {
        return Parse_Gemini_Response(responseBuffer, bufferSize);
    }

    return false;
}

bool Net_QueryGeminiAudio(const char *apiKey, const char *promt, u8 *audioData, u32 audioSize, char *responseBuffer, size_t bufferSize) {
    char *jsonBody = Create_JSON(promt, audioData, audioSize);
    if (!jsonBody) return false;

    bool netSuccess = Perform_CURL_Request(apiKey, jsonBody, responseBuffer, bufferSize);
    free(jsonBody);

    if (netSuccess) {
        return Parse_Gemini_Response(responseBuffer, bufferSize);
    }

    return false;
}
