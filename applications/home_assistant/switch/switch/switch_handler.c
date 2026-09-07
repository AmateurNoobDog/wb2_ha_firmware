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

static void switch_push_state(void)
{
    uint8_t mac[6];
    char state[192];
    char full_json[256];

    if (!ha_push_enabled()) {
        return;
    }
    switch_handler_get_state(state, sizeof(state));
    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    snprintf(full_json, sizeof(full_json),
             "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
             "\"type\":\"%s\",\"name\":\"%s\",%s}",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             DEVICE_TYPE, DEVICE_NAME, state);
    ha_push_send(full_json);
}

int switch_handler_get_state(char *buf, int buf_len)
{
    int i;
    int used = 0;

    used += snprintf(buf + used, buf_len - used, "\"model\":\"%s\",\"sw_version\":\"%s\",\"count\":%d",
                     DEVICE_MODEL, DEVICE_SW_VERSION, SWITCH_COUNT);
    used += snprintf(buf + used, buf_len - used, ",\"names\":[");
    for (i = 0; i < SWITCH_COUNT; i++) {
        used += snprintf(buf + used, buf_len - used, "\"%s\"%s",
                         s_names[i], i < SWITCH_COUNT - 1 ? "," : "");
    }
    used += snprintf(buf + used, buf_len - used, "]");
    for (i = 0; i < SWITCH_COUNT; i++) {
        used += snprintf(buf + used, buf_len - used,
                         ",\"%s\":%d",
                         i == 0 ? "on" : (i == 1 ? "on1" : "on2"),
                         relay_get(i) ? 1 : 0);
        if (used >= buf_len) {
            break;
        }
    }
    used += snprintf(buf + used, buf_len - used, ",\"push\":%d", ha_push_enabled());
    return used;
}

int switch_handler_set_state(const char *cmd_json)
{
    int i;
    int v;
    int changed = 0;

    const char *cmd = ha_json_str(cmd_json, "cmd");
    if (cmd && strncmp(cmd, "push_cfg", 8) == 0) {
        const char *ip = ha_json_str(cmd_json, "ip");
        int port = ha_json_int(cmd_json, "port", HA_PUSH_DEFAULT_PORT);
        if (ip) {
            ha_push_set_target(ip, (uint16_t)port);
        }
        return 0;
    }

    for (i = 0; i < SWITCH_COUNT; i++) {
        v = ha_json_int(cmd_json,
                        i == 0 ? "on" : (i == 1 ? "on1" : "on2"), -1);
        if (v >= 0) {
            bool old = relay_get(i);
            relay_set(i, v != 0);
            if (old != (v != 0)) {
                changed = 1;
            }
            blog_info("[TCP] set sw%d on=%d", i, v != 0);
        }
    }

    if (changed) {
        switch_push_state();
    }
    return 0;
}
