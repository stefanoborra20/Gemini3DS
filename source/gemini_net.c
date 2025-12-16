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
} ResponseData;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realSize = size * nmemb;
    ResponseData *mem = (ResponseData *)userp;

    strncat(mem->memory, (char*)contents, realSize);
    mem->size += realSize;
    return realSize;
}

bool Net_QueryGemini(const char *apiKey, const char *promt, char *responseBuffer, size_t bufferSize) {
    CURL *curl;
    CURLcode res;
    bool success = false;

    /* --- Build Json --- */
    json_t *root = json_object();
    json_t *contents = json_array();
    json_t *content_obj = json_object();
    json_t *parts = json_array();
    json_t *part_obj = json_object();
    
    /* --- User content --- */
    json_object_set_new(part_obj, "text", json_string(promt));
    json_array_append_new(parts, part_obj);
    json_object_set_new(content_obj, "parts", parts);
    json_array_append_new(contents, content_obj);
    json_object_set_new(root, "contents", contents);

    /* --- Config --- */
    json_t *gen_config = json_object();
    float temp = Settings_GetTemperature();
    int max_tokens = Settings_GetMaxTokens();
    
    json_object_set_new(gen_config, "temperature", json_real(temp));
    json_object_set_new(gen_config, "maxOutputTokens", json_integer(max_tokens));

    /* Attach config to root */
    json_object_set_new(root, "generationConfig", gen_config);

    char *json_body = json_dumps(root, 0);

    // Build URL
    char url[512];
    snprintf(url, sizeof(url),
           "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
           Settings_GetModel(),
           apiKey);

    curl = curl_easy_init();
    if (curl) {
        ResponseData chunk;
        chunk.memory = responseBuffer;
        chunk.size = 0;
        responseBuffer[0] = '\0';
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &chunk);

        // Disable SSL
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        // Send Request
        res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res != CURLE_OK) {
            snprintf(responseBuffer, bufferSize, "Curl Error:%s", curl_easy_strerror(res));
        } else if (http_code != 200) {
            char raw_err[128];
            strncpy(raw_err, responseBuffer, 127);
            raw_err[127] = '\0';
            snprintf(responseBuffer, bufferSize, "HTTP: %ld Err: %.100s", http_code, raw_err);
        } else {
            // HTTP OK
            json_error_t error;
            json_t *resp_root = json_loads(responseBuffer, 0, &error);

            if (resp_root) {
                json_t *candidates = json_object_get(resp_root, "candidates");
                json_t *cand0 = json_array_get(candidates, 0);
                json_t *content = json_object_get(cand0, "content");
                json_t *r_parts = json_object_get(content, "parts");
                json_t *part0 = json_array_get(r_parts, 0);
                json_t *text_obj = json_object_get(part0, "text");

                const char *text_content = json_string_value(text_obj);

                if (text_content) {
                    char temp[bufferSize];
                    strncpy(temp, text_content, bufferSize);
                    strncpy(responseBuffer, temp, bufferSize);
                    success = true;
                } else {
                    snprintf(responseBuffer, bufferSize, "Error: Invalid JSON");
                }
                json_decref(resp_root);
            }
        }
        
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    free(json_body);
    json_decref(root);
    return success;
}
