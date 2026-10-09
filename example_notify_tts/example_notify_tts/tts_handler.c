#include <stdio.h>
#include <string.h>

#include "blog.h"
#include "ha_device.h"
#include "ha_json.h"
#include "ha_push.h"
#include "tts_handler.h"
#include "tw_tts_app.h"
#include "app_config.h"
#include "store.h"
#include <wifi_mgmr_ext.h>

static uint8_t s_volume = TTS_DEFAULT_VOLUME;
static uint8_t s_speed = TTS_DEFAULT_SPEED;

void tts_handler_init(void)
{
    store_tts_load(&s_volume, &s_speed);
    blog_info("[TTS] loaded: volume=%d speed=%d", s_volume, s_speed);
}

static void gen_entity_id(char *buf, int buf_len, const uint8_t *mac, const char *suffix)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], suffix);
}

static void tts_push_state(void)
{
    uint8_t mac[6];
    char id[20];
    char json[256];
    int used;

    if (!ha_push_enabled()) {
        return;
    }

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used = snprintf(json, sizeof(json), "{\"entities\":[");
    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_VOLUME);
    used += snprintf(json + used, sizeof(json) - used,
                     "{\"id\":\"%s\",\"type\":\"number\",\"value\":%d}",
                     id, s_volume);
    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_SPEED);
    used += snprintf(json + used, sizeof(json) - used,
                     ",{\"id\":\"%s\",\"type\":\"number\",\"value\":%d}",
                     id, s_speed);
    used += snprintf(json + used, sizeof(json) - used, "]}");

    ha_push_send(json);
}

int tts_handler_get_device(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");

    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_NOTIFY);
    used += snprintf(buf + used, buf_len - used,
                     "{\"id\":\"%s\",\"type\":\"notify\",\"name\":\"TTS\","
                     "\"icon\":\"mdi:speaker-message\"}", id);

    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_VOLUME);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"number\","
                     "\"name\":\"\xE9\x9F\xB3\xE9\x87\x8F\","
                     "\"icon\":\"mdi:volume-high\","
                     "\"min\":%d,\"max\":%d,\"step\":%d}",
                     id, TTS_VOLUME_MIN, TTS_VOLUME_MAX, TTS_VOLUME_STEP);

    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_SPEED);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"number\","
                     "\"name\":\"\xE8\xAF\xAD\xE9\x80\x9F\","
                     "\"icon\":\"mdi:volume-vibrate\","
                     "\"min\":%d,\"max\":%d,\"step\":%d}",
                     id, TTS_SPEED_MIN, TTS_SPEED_MAX, TTS_SPEED_STEP);

    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int tts_handler_get_state(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");

    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_VOLUME);
    used += snprintf(buf + used, buf_len - used,
                     "{\"id\":\"%s\",\"type\":\"number\",\"value\":%d}",
                     id, s_volume);

    gen_entity_id(id, sizeof(id), mac, TTS_ENTITY_SPEED);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"number\",\"value\":%d}",
                     id, s_speed);

    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int tts_handler_set_state(const char *cmd_json)
{
    int changed = 0;

    const char *cmd = ha_json_str(cmd_json, "cmd");
    if (cmd && strcmp(cmd, "push_cfg") == 0) {
        const char *host = ha_json_str(cmd_json, "host");
        if (!host) host = ha_json_str(cmd_json, "ip");
        int port = ha_json_int(cmd_json, "port", HA_PUSH_DEFAULT_PORT);
        if (host) {
            ha_push_set_target(host, (uint16_t)port);
        }
        return 0;
    }

    const char *id = ha_json_str(cmd_json, "id");
    if (id) {
        int id_len = strlen(id);

        if (id_len >= 3) {
            const char *suffix = id + id_len - 3;

            if (strcmp(suffix, TTS_ENTITY_NOTIFY) == 0) {
                const char *text = ha_json_str(cmd_json, "text");
                if (text) {
                    blog_info("[TCP] TTS: %s", text);
                    TTS(text);
                }
                return 0;
            }

            if (strcmp(suffix, TTS_ENTITY_VOLUME) == 0) {
                int v = ha_json_int(cmd_json, "value", -1);
                if (v >= TTS_VOLUME_MIN && v <= TTS_VOLUME_MAX) {
                    s_volume = (uint8_t)v;
                    blog_info("[TCP] volume: %d", s_volume);
                    TTS_set_volume(s_volume);
                    changed = 1;
                }
            }

            if (strcmp(suffix, TTS_ENTITY_SPEED) == 0) {
                int v = ha_json_int(cmd_json, "value", -1);
                if (v >= TTS_SPEED_MIN && v <= TTS_SPEED_MAX) {
                    s_speed = (uint8_t)v;
                    blog_info("[TCP] speed: %d", s_speed);
                    TTS_set_speed(s_speed);
                    changed = 1;
                }
            }
        }
    }

    if (changed) {
        store_tts_save(s_volume, s_speed);
        tts_push_state();
    }
    return 0;
}
