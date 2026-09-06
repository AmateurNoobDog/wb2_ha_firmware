#include <stdio.h>
#include <string.h>

#include "blog.h"
#include "ha_device.h"
#include "ha_json.h"
#include "led_handler.h"
#include "led.h"
#include "app_config.h"

int led_handler_get_state(char *buf, int buf_len)
{
    uint8_t r, g, b, brightness;

    led_get_state(&r, &g, &b, &brightness);
    return snprintf(buf, buf_len,
                    "\"model\":\"%s\",\"sw_version\":\"%s\","
                    "\"r\":%d,\"g\":%d,\"b\":%d,\"brightness\":%d",
                    DEVICE_MODEL, DEVICE_SW_VERSION, r, g, b, brightness);
}

int led_handler_set_state(const char *cmd_json)
{
    uint8_t r, g, b, brightness;
    int v;

    led_get_state(&r, &g, &b, &brightness);

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
    return 0;
}
