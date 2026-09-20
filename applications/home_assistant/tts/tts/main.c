#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aos/yloop.h>
#include "blog.h"

#include <lwip/tcpip.h>
#include "uart_app.h"
#include "tw_tts_app.h"
#include "store.h"
#include "wifi_sta.h"
#include "blufi_app.h"
#include "ha_device.h"
#include "ha_push.h"
#include "ha_mdns.h"
#include "tts_handler.h"
#include "app_config.h"

static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .model = DEVICE_MODEL,
    .manufacturer = DEVICE_MANUFACTURER,
    .sw_version = DEVICE_SW_VERSION,
    .port = TCP_SERVER_PORT,
    .get_device = tts_handler_get_device,
    .get_state = tts_handler_get_state,
    .set_state = tts_handler_set_state,
};

static void on_got_ip(void)
{
    store_reboot_provision_clear();
    blog_info("[APP] got ip, starting tcp json server");
    blog_info("[SYS] Memory left is %d Bytes", xPortGetFreeHeapSize());
    ha_push_init();
    ha_mdns_start();
    xTaskCreate(ha_tcp_server_start, (char *)"tcp_json", TCP_SERVER_STACK, (void *)&ha_dev, 15, NULL);
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
    uart_init();
    uart_log("[TTS] uart initialized\r\n");
    uart_log("[TTS] starting Ai-WB2 TTS firmware\r\n");
    tts_handler_init();
    uart_log("[TTS] tts_handler initialized\r\n");
    tcpip_init(NULL, NULL);
    uart_log("[TTS] tcpip initialized\r\n");
    boot_mode();
    uart_log("[TTS] boot_mode done, entering main loop\r\n");
    while (1) vTaskDelay(1000);
}
