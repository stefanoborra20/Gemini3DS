#include "gemini_net.h"
#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <curl/curl.h>
#include <jansson.h>

#define SOC_ALIGN 0x1000
#define SOC_BUFFER_SIZE 0x100000
#define GEMINI_ENDPOINT "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent"

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
    
    json_t *root = json_object();
    json_t *contents = json_array();
    json_t *content_obj = json_object();
    json_t *parts = json_array();
    json_t *part_obj = json_object();
    
    json_t *sys_inst = json_object();
    json_t *sys_parts = json_array();
    json_t *sys_part = json_object();
    
    // SYSTEM INSTRUCTIONS 
    const char *rules = "You are a text-only terminal assistant."
        "Return strict plain text. Do NOT use markdown formatting.";

    json_object_set_new(sys_part, "text", json_string(rules));
    json_array_append(sys_parts, sys_part);
    json_object_set_new(sys_inst, "parts", sys_parts);
    json_object_set_new(root, "systemInstruction", sys_inst);
    
    // GENERATION CONFIG 
    json_t *gen_config = json_object();
    json_object_set_new(gen_config, "responseMimeType", json_string("text/plain"));
    json_object_set_new(root, "generationConfig", gen_config);

    // USER REQUEST
    json_object_set_new(part_obj, "text", json_string(promt));
    json_array_append_new(parts, part_obj);
    json_object_set_new(content_obj, "parts", parts);
    json_array_append_new(contents, content_obj);
    json_object_set_new(root, "contents", contents);

    char *json_body = json_dumps(root, 0);

    char url[512];
    snprintf(url, sizeof(url), "%s?key=%s", GEMINI_ENDPOINT, apiKey);

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

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            snprintf(responseBuffer, bufferSize, "Curl Error:%s", curl_easy_strerror(res));
        } else {
            // JSON parsing
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
                    // copy clean text in buffer
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
