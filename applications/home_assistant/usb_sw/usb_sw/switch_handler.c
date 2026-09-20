#include <stdio.h>
#include <string.h>

#include "blog.h"
#include "ha_device.h"
#include "ha_json.h"
#include "ha_push.h"
#include "switch_handler.h"
#include "relay.h"
#include "app_config.h"
#include <wifi_mgmr_ext.h>

static const char *const s_names[SWITCH_COUNT] = SWITCH_NAMES;

static void gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%03d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], seq);
}

static void switch_push_state(void)
{
    uint8_t mac[6];
    char id[20];
    char json[192];
    int used;

    if (!ha_push_enabled()) {
        return;
    }

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used = snprintf(json, sizeof(json), "{\"entities\":[");
    for (int i = 0; i < SWITCH_COUNT; i++) {
        gen_entity_id(id, sizeof(id), mac, i + 1);
        used += snprintf(json + used, sizeof(json) - used,
                         "%s{\"id\":\"%s\",\"type\":\"switch\",\"on\":%d}",
                         i > 0 ? "," : "", id, relay_get(i) ? 1 : 0);
    }
    used += snprintf(json + used, sizeof(json) - used, "]}");

    ha_push_send(json);
}

int switch_handler_get_device(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");
    for (int i = 0; i < SWITCH_COUNT; i++) {
        gen_entity_id(id, sizeof(id), mac, i + 1);
        used += snprintf(buf + used, buf_len - used,
                         "%s{\"id\":\"%s\",\"type\":\"switch\",\"name\":\"%s\","
                         "\"icon\":\"mdi:toggle-switch\"}",
                         i > 0 ? "," : "", id, s_names[i]);
    }
    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int switch_handler_get_state(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");
    for (int i = 0; i < SWITCH_COUNT; i++) {
        gen_entity_id(id, sizeof(id), mac, i + 1);
        used += snprintf(buf + used, buf_len - used,
                         "%s{\"id\":\"%s\",\"type\":\"switch\",\"on\":%d}",
                         i > 0 ? "," : "", id, relay_get(i) ? 1 : 0);
    }
    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int switch_handler_set_state(const char *cmd_json)
{
    int i;
    int v;
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
    v = ha_json_int(cmd_json, "on", -1);
    if (id && v >= 0) {
        int id_len = strlen(id);
        if (id_len >= 3) {
            int seq = (id[id_len - 3] - '0') * 100 +
                      (id[id_len - 2] - '0') * 10 +
                      (id[id_len - 1] - '0');
            i = seq - 1;
            if (i >= 0 && i < SWITCH_COUNT) {
                bool old = relay_get(i);
                relay_set(i, v != 0);
                if (old != (v != 0)) {
                    changed = 1;
                }
                blog_info("[TCP] set sw%d on=%d", i, v != 0);
            }
        }
    } else {
        for (i = 0; i < SWITCH_COUNT; i++) {
            v = ha_json_int(cmd_json, "on", -1);
            if (i == 1) {
                v = ha_json_int(cmd_json, "on1", -1);
            } else if (i == 2) {
                v = ha_json_int(cmd_json, "on2", -1);
            }
            if (v >= 0) {
                bool old = relay_get(i);
                relay_set(i, v != 0);
                if (old != (v != 0)) {
                    changed = 1;
                }
                blog_info("[TCP] set sw%d on=%d", i, v != 0);
            }
        }
    }

    if (changed) {
        switch_push_state();
    }
    return 0;
}
