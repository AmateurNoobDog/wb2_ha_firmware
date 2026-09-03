#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aos/yloop.h>
#include "blog.h"

#include "led.h"
#include "store.h"
#include "wifi_sta.h"
#include "blufi_app.h"
#include "ha_device.h"
#include "led_handler.h"
#include "app_config.h"

static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .port = TCP_SERVER_PORT,
    .get_state = led_handler_get_state,
    .set_state = led_handler_set_state,
};

static void on_got_ip(void)
{
    blog_info("[APP] got ip, starting tcp json server");
    blog_info("[SYS] Memory left is %d Bytes", xPortGetFreeHeapSize());
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

    if (store_wifi_load(&cfg)) {
        blog_info("[APP] wifi ssid: %s", cfg.ssid);
        blog_info("[APP] connecting wifi");
        wifi_sta_start(cfg.ssid, cfg.pwd);
    } else {
        blog_info("[APP] no wifi config, enter blufi provisioning");
        blufi_app_start();
    }
}

void main(void)
{
    led_init();
    puts("[OS] light starting...");
    boot_mode();
    puts("[OS] main exit");
}