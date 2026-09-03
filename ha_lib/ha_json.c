#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ha_json.h"

const char *ha_json_str(const char *buf, const char *key)
{
    char pattern[16];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    p = strstr(buf, pattern);
    if (p == NULL) {
        return NULL;
    }
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '"') {
        p++;
    }
    return p;
}

int ha_json_int(const char *buf, const char *key, int def)
{
    const char *p = ha_json_str(buf, key);

    if (p == NULL || ((*p < '0' || *p > '9') && *p != '-')) {
        return def;
    }
    return atoi(p);
}