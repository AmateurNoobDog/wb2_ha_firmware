#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <bl_gpio.h>

#include "ha_device.h"
#include "ha_json.h"
#include "ha_push.h"
#include "event_handler.h"
#include "uart_433.h"
#include "app_config.h"
#include <wifi_mgmr_ext.h>

#define KEY_LEN             6
#define UART_BUF_SIZE       32
#define DEBOUNCE_MS         100

static volatile uint32_t s_tick = 0;
static volatile uint8_t s_pair_event = 0;

typedef struct {
    char key[KEY_LEN + 1];
    char event_type[8];
    uint32_t timestamp;
} event_433_t;

static event_433_t s_last_event = { .key = "", .event_type = "", .timestamp = 0 };

static void tick_task(void *arg)
{
    (void)arg;
    while (1) {
        s_tick++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void log_hex(const char *tag, const uint8_t *data, int len)
{
    char buf[256];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s hex:", tag);
    for (int i = 0; i < len && pos < (int)sizeof(buf) - 4; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, " %02X", data[i]);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "\r\n");
    uart_433_log(buf);
}

static void pair_button_click(void)
{
    uart_433_log("[433] pair button click\r\n");
    s_pair_event = 1;

    bl_gpio_enable_output(PAIR_IO, 1, 0);
    bl_gpio_output_set(PAIR_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    bl_gpio_output_set(PAIR_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    bl_gpio_output_set(PAIR_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    bl_gpio_output_set(PAIR_IO, 1);

    bl_gpio_enable_input(PAIR_IO, 1, 0);
    uint32_t wait = 0;
    while (wait < 200) {
        if (!bl_gpio_input_get_value(PAIR_IO)) {
            wait = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        wait++;
    }

    uint8_t buf[16];
    uart_433_read(buf, sizeof(buf));
    s_pair_event = 0;
    uart_433_log("[433] pair done\r\n");
}

static void reset_button_long_press(void)
{
    uart_433_log("[433] reset button long press\r\n");
    s_pair_event = 1;

    bl_gpio_enable_output(PAIR_IO, 1, 0);
    bl_gpio_output_set(PAIR_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(10000));
    bl_gpio_output_set(PAIR_IO, 1);

    bl_gpio_enable_input(PAIR_IO, 1, 0);
    uint32_t wait = 0;
    while (wait < 200) {
        if (!bl_gpio_input_get_value(PAIR_IO)) {
            wait = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        wait++;
    }

    uint8_t buf[16];
    uart_433_read(buf, sizeof(buf));
    s_pair_event = 0;
    uart_433_log("[433] reset done\r\n");
}

static void gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%03d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], seq);
}

static void event_433_push(const char *event_type, const char *key)
{
    uint8_t mac[6];
    char id[20];
    char json[128];

    if (!ha_push_enabled()) {
        return;
    }

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    gen_entity_id(id, sizeof(id), mac, 3);

    snprintf(json, sizeof(json),
             "{\"id\":\"%s\",\"type\":\"event\","
             "\"event_type\":\"%s\",\"event_id\":\"%s\"}",
             id, event_type, key);

    uart_433_log("[433] push: ");
    uart_433_log(json);
    uart_433_log("\r\n");
    ha_push_send(json);
}

static void uart_read_task(void *arg)
{
    (void)arg;
    uint8_t tmp[UART_BUF_SIZE];
    uint8_t key[KEY_LEN + 1];
    char last_key[KEY_LEN + 1] = "";
    char log_buf[128];

    uart_433_log("[433] uart_read_task started\r\n");
    while (1) {
        int len = uart_433_read(tmp, sizeof(tmp));
        if (len <= 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        int total = len;

        vTaskDelay(pdMS_TO_TICKS(40));

        len = uart_433_read(tmp + total, sizeof(tmp) - total);
        if (len > 0) {
            total += len;
        }

        snprintf(log_buf, sizeof(log_buf), "[433] uart got %d bytes\r\n", total);
        uart_433_log(log_buf);
        log_hex("[433] raw", tmp, total);

        if (s_pair_event) {
            uart_433_log("[433] in pair mode, skip\r\n");
            continue;
        }

        if (total < 9) {
            snprintf(log_buf, sizeof(log_buf), "[433] data too short: %d\r\n", total);
            uart_433_log(log_buf);
            continue;
        }

        memcpy(key, tmp + 3, KEY_LEN);
        key[KEY_LEN] = '\0';

        snprintf(log_buf, sizeof(log_buf), "[433] key: %.6s\r\n", key);
        uart_433_log(log_buf);

        int changed = 0;
        for (int i = 0; i < KEY_LEN; i++) {
            if (key[i] != (uint8_t)last_key[i]) {
                changed = 1;
                break;
            }
        }

        if (!changed && (s_tick - s_last_event.timestamp) < DEBOUNCE_MS) {
            continue;
        }

        memcpy(s_last_event.key, key, KEY_LEN + 1);
        s_last_event.timestamp = s_tick;
        memcpy(last_key, key, KEY_LEN + 1);

        event_433_push("press", (const char *)key);
        strncpy(s_last_event.event_type, "press", sizeof(s_last_event.event_type));
        vTaskDelay(pdMS_TO_TICKS(10));
        event_433_push("release", (const char *)key);
        strncpy(s_last_event.event_type, "release", sizeof(s_last_event.event_type));
    }
}

void event_handler_init(void)
{
    uart_433_init();
    xTaskCreate(tick_task, "tick", 1024, NULL, 5, NULL);
    xTaskCreate(uart_read_task, "uart_433", 2048, NULL, 5, NULL);
    uart_433_log("[433] event handler initialized\r\n");
}

int event_handler_get_device(char *buf, int buf_len)
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
                     "{\"id\":\"%s\",\"type\":\"button\",\"name\":\"配对\","
                     "\"icon\":\"mdi:remote\",\"action\":\"pair\"}", id);

    gen_entity_id(id, sizeof(id), mac, 2);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"button\",\"name\":\"重置\","
                     "\"icon\":\"mdi:restore\",\"action\":\"reset\"}", id);

    gen_entity_id(id, sizeof(id), mac, 3);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"event\",\"name\":\"键值\","
                     "\"icon\":\"mdi:remote\"}", id);

    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int event_handler_get_state(char *buf, int buf_len)
{
    if (s_last_event.key[0] == '\0') {
        buf[0] = '\0';
        return 0;
    }

    uint8_t mac[6];
    char id[20];

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    gen_entity_id(id, sizeof(id), mac, 3);

    return snprintf(buf, buf_len,
                    "\"entities\":["
                    "{\"id\":\"%s\",\"type\":\"event\","
                    "\"event_type\":\"%s\",\"event_id\":\"%s\"}"
                    "]",
                    id, s_last_event.event_type, s_last_event.key);
}

int event_handler_set_state(const char *cmd_json)
{
    const char *cmd = ha_json_str(cmd_json, "cmd");
    if (cmd == NULL) {
        return -1;
    }

    if (strcmp(cmd, "pair") == 0) {
        uart_433_log("[433] cmd: pair\r\n");
        pair_button_click();
        return 0;
    }

    if (strcmp(cmd, "reset") == 0) {
        uart_433_log("[433] cmd: reset\r\n");
        reset_button_long_press();
        return 0;
    }

    if (strcmp(cmd, "push_cfg") == 0) {
        const char *host = ha_json_str(cmd_json, "host");
        if (!host) host = ha_json_str(cmd_json, "ip");
        int port = ha_json_int(cmd_json, "port", HA_PUSH_DEFAULT_PORT);
        if (host) {
            ha_push_set_target(host, (uint16_t)port);
        }
        return 0;
    }

    uart_433_log("[433] unknown cmd\r\n");
    return -1;
}
