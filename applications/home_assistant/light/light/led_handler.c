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

static void led_push_state(void)
{
    uint8_t mac[6];
    char state[192];
    char full_json[256];

    if (!ha_push_enabled()) {
        return;
    }
    led_handler_get_state(state, sizeof(state));
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

int led_handler_get_state(char *buf, int buf_len)
{
    uint8_t r, g, b, brightness;

    led_get_state(&r, &g, &b, &brightness);
    return snprintf(buf, buf_len,
                    "\"model\":\"%s\",\"sw_version\":\"%s\","
                    "\"r\":%d,\"g\":%d,\"b\":%d,\"brightness\":%d,\"push\":%d",
                    DEVICE_MODEL, DEVICE_SW_VERSION, r, g, b, brightness,
                    ha_push_enabled());
}

int led_handler_set_state(const char *cmd_json)
{
    uint8_t old_r, old_g, old_b, old_br;
    uint8_t r, g, b, brightness;
    int v;

    const char *cmd = ha_json_str(cmd_json, "cmd");
    if (cmd && strncmp(cmd, "push_cfg", 8) == 0) {
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
