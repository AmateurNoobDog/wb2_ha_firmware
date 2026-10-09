#include <stdio.h>
#include <string.h>
#include <math.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aos/yloop.h>
#include "blog.h"

#include <lwip/tcpip.h>
#include "store.h"
#include "wifi_sta.h"
#include "blufi_app.h"
#include "ha_device.h"
#include "ha_push.h"
#include "ha_mdns.h"
#include "dht20.h"
#include "app_config.h"
#include <wifi_mgmr_ext.h>

static float s_last_temperature = -999.0f;
static float s_last_humidity = -999.0f;

static void gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%03d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], seq);
}

static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .model = DEVICE_MODEL,
    .manufacturer = DEVICE_MANUFACTURER,
    .sw_version = DEVICE_SW_VERSION,
    .port = TCP_SERVER_PORT,
    .get_device = NULL,
    .get_state = NULL,
    .set_state = NULL,
};

static int get_device_impl(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    gen_entity_id(id, sizeof(id), mac, 1);
    used += snprintf(buf + used, buf_len - used,
        "\"entities\":["
        "{\"id\":\"%s\",\"type\":\"sensor\",\"name\":\"温度\",\"icon\":\"mdi:thermometer\","
        "\"device_class\":\"temperature\",\"unit\":\"°C\"},",
        id);

    gen_entity_id(id, sizeof(id), mac, 2);
    used += snprintf(buf + used, buf_len - used,
        "{\"id\":\"%s\",\"type\":\"sensor\",\"name\":\"湿度\",\"icon\":\"mdi:water-percent\","
        "\"device_class\":\"humidity\",\"unit\":\"%%\"}"
        "]",
        id);

    return used;
}

static int get_state_impl(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    float t, h;

    if (DHT20_Read(&t, &h) != 0) {
        blog_error("[SENSOR] read failed");
        return snprintf(buf, buf_len, "\"entities\":[]");
    }

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    int used = 0;
    used += snprintf(buf + used, buf_len - used, "\"entities\":[");

    gen_entity_id(id, sizeof(id), mac, 1);
    used += snprintf(buf + used, buf_len - used,
        "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%.1f},", id, t);

    gen_entity_id(id, sizeof(id), mac, 2);
    used += snprintf(buf + used, buf_len - used,
        "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%.1f}]", id, h);

    return used;
}

static ha_device_t ha_dev_runtime;

static void sensor_task(void *arg)
{
    (void)arg;
    float temperature, humidity;
    char push_json[256];

    for (;;) {
        if (DHT20_Read(&temperature, &humidity) != 0) {
            blog_error("[SENSOR] dht20 read failed");
            vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
            continue;
        }

        blog_info("[SENSOR] temp=%.1fC hum=%.1f%%", temperature, humidity);

        int temp_changed = (fabsf(temperature - s_last_temperature) >= 0.1f);
        int hum_changed = (fabsf(humidity - s_last_humidity) >= 0.1f);

        if (temp_changed || hum_changed) {
            uint8_t mac[6];
            char id_t[20], id_h[20];
            if (wifi_mgmr_sta_mac_get(mac) != 0) {
                memset(mac, 0, sizeof(mac));
            }
            gen_entity_id(id_t, sizeof(id_t), mac, 1);
            gen_entity_id(id_h, sizeof(id_h), mac, 2);

            int n = snprintf(push_json, sizeof(push_json),
                "{\"entities\":["
                "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%.1f},"
                "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%.1f}"
                "]}",
                id_t, temperature, id_h, humidity);
            push_json[n] = '\0';
            ha_push_send(push_json);
            s_last_temperature = temperature;
            s_last_humidity = humidity;
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

static void on_got_ip(void)
{
    store_reboot_provision_clear();
    blog_info("[APP] got ip, starting tcp json server");
    blog_info("[SYS] Memory left is %d Bytes", xPortGetFreeHeapSize());

    ha_dev_runtime = ha_dev;
    ha_dev_runtime.get_device = get_device_impl;
    ha_dev_runtime.get_state = get_state_impl;

    ha_push_init();
    ha_mdns_start();
    xTaskCreate(ha_tcp_server_start, (char *)"tcp_json", TCP_SERVER_STACK, (void *)&ha_dev_runtime, 15, NULL);
    xTaskCreate(sensor_task, (char *)"sensor", 2048, NULL, 14, NULL);
}

static void app_evt_cb(input_event_t *event, void *private_data)
{
    switch (event->code) {
    case CODE_WIFI_ON_GOT_IP:
        on_got_ip();
        break;

    default:
        break;
    }
}

static void boot_mode(void)
{
    store_wifi_t cfg;

    store_init();
    aos_register_event_filter(EV_WIFI, app_evt_cb, NULL);

    if (store_reboot_provision_check(REBOOT_PROVISION_COUNT)) {
        blog_info("[APP] %d consecutive reboots, enter provisioning mode", REBOOT_PROVISION_COUNT);
        store_reboot_provision_clear();
        blufi_app_start();
        return;
    }

    if (store_wifi_load(&cfg)) {
        blog_info("[APP] wifi ssid: %s", cfg.ssid);
        blog_info("[APP] connecting wifi");
        wifi_sta_start(cfg.ssid, cfg.pwd);
    } else {
        blog_info("[APP] no wifi config, enter blufi provisioning");
        store_reboot_provision_clear();
        blufi_app_start();
    }
}

void main(void)
{
    DHT20_Init();
    puts("[OS] dht20_ha starting...");
    tcpip_init(NULL, NULL);
    boot_mode();
    puts("[OS] main exit");
}
