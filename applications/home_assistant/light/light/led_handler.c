#include <stdio.h>
#include <string.h>

#include "blog.h"
#include "ha_device.h"
#include "ha_json.h"
#include "ha_push.h"
#include "led_handler.h"
#include "led.h"
#include "app_config.h"
#include <wifi_mgmr_ext.h>

static void gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%03d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], seq);
}

static void led_push_state(void)
{
    uint8_t mac[6];
    char id[20];
    char json[128];
    uint8_t r, g, b, brightness;

    if (!ha_push_enabled()) {
        return;
    }

    led_get_state(&r, &g, &b, &brightness);

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    gen_entity_id(id, sizeof(id), mac, 1);

    snprintf(json, sizeof(json),
             "{\"id\":\"%s\",\"type\":\"light\","
             "\"r\":%d,\"g\":%d,\"b\":%d,\"brightness\":%d}",
             id, r, g, b, brightness);
    ha_push_send(json);
}

int led_handler_get_device(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");

    gen_entity_id(id, sizeof(id), mac, 1);
    used += snprintf(buf + used, buf_len - used,
                     "{\"id\":\"%s\",\"type\":\"light\",\"name\":\"彩灯\","
                     "\"icon\":\"mdi:lightbulb\"}", id);

    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int led_handler_get_state(char *buf, int buf_len)
{
    uint8_t r, g, b, brightness;
    uint8_t mac[6];
    char id[20];

    led_get_state(&r, &g, &b, &brightness);

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    gen_entity_id(id, sizeof(id), mac, 1);

    return snprintf(buf, buf_len,
                    "\"entities\":["
                    "{\"id\":\"%s\",\"type\":\"light\","
                    "\"r\":%d,\"g\":%d,\"b\":%d,\"brightness\":%d}"
                    "]",
                    id, r, g, b, brightness);
}

int led_handler_set_state(const char *cmd_json)
{
    uint8_t old_r, old_g, old_b, old_br;
    uint8_t r, g, b, brightness;
    int v;

    const char *cmd = ha_json_str(cmd_json, "cmd");
    if (cmd && strcmp(cmd, "push_cfg") == 0) {
        const char *ip = ha_json_str(cmd_json, "ip");
        int port = ha_json_int(cmd_json, "port", HA_PUSH_DEFAULT_PORT);
        if (ip) {
            ha_push_set_target(ip, (uint16_t)port);
        }
        return 0;
    }

    led_get_state(&old_r, &old_g, &old_b, &old_br);
    r = old_r;
    g = old_g;
    b = old_b;
    brightness = old_br;

    v = ha_json_int(cmd_json, "r", -1);
    if (v >= 0) {
        r = (uint8_t)(v > 255 ? 255 : v);
    }
    v = ha_json_int(cmd_json, "g", -1);
    if (v >= 0) {
        g = (uint8_t)(v > 255 ? 255 : v);
    }
    v = ha_json_int(cmd_json, "b", -1);
    if (v >= 0) {
        b = (uint8_t)(v > 255 ? 255 : v);
    }
    v = ha_json_int(cmd_json, "brightness", -1);
    if (v >= 0) {
        brightness = (uint8_t)(v > 255 ? 255 : v);
    }

    led_set_state(r, g, b, brightness);
    blog_info("[TCP] set r=%d g=%d b=%d brightness=%d", r, g, b, brightness);

    if (r != old_r || g != old_g || b != old_b || brightness != old_br) {
        led_push_state();
    }
    return 0;
}
