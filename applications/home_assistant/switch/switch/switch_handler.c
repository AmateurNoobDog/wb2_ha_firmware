#include <stdio.h>

#include "blog.h"
#include "ha_device.h"
#include "ha_json.h"
#include "switch_handler.h"
#include "relay.h"
#include "app_config.h"

static const char *const s_names[SWITCH_COUNT] = SWITCH_NAMES;

int switch_handler_get_state(char *buf, int buf_len)
{
    int i;
    int used = 0;

    used += snprintf(buf + used, buf_len - used, "\"model\":\"%s\",\"count\":%d",
                     DEVICE_MODEL, SWITCH_COUNT);
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
    return used;
}

int switch_handler_set_state(const char *cmd_json)
{
    int i;
    int v;

    for (i = 0; i < SWITCH_COUNT; i++) {
        v = ha_json_int(cmd_json,
                        i == 0 ? "on" : (i == 1 ? "on1" : "on2"), -1);
        if (v >= 0) {
            relay_set(i, v != 0);
            blog_info("[TCP] set sw%d on=%d", i, v != 0);
        }
    }
    return 0;
}