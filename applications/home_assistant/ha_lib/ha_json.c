#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include "ha_json.h"

const char *ha_json_str(const char *buf, const char *key)
{
    static char s_buf[128];
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        return NULL;
    }
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (!item || item->type != cJSON_String) {
        cJSON_Delete(root);
        return NULL;
    }
    strncpy(s_buf, item->valuestring, sizeof(s_buf) - 1);
    s_buf[sizeof(s_buf) - 1] = '\0';
    cJSON_Delete(root);
    return s_buf;
}

int ha_json_int(const char *buf, const char *key, int def)
{
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        return def;
    }
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (!item || item->type != cJSON_Number) {
        cJSON_Delete(root);
        return def;
    }
    int val = item->valueint;
    cJSON_Delete(root);
    return val;
}
