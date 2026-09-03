#include <string.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aos/yloop.h>
#include <hal_wifi.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include "blog.h"

#include "wifi_sta.h"
#include "app_config.h"

static char s_ssid[STORE_SSID_MAX];
static char s_pwd[STORE_PWD_MAX];

static wifi_conf_t conf =
{
    .country_code = "CN",
};

static void wifi_sta_ipv4_print(void)
{
    uint32_t ip = 0, gw = 0, mask = 0;

    if (wifi_mgmr_sta_ip_get(&ip, &gw, &mask) == 0) {
        blog_info("[NET] ip: %s, gw: %s, mask: %s",
                  ip4addr_ntoa((ip4_addr_t *)&ip),
                  ip4addr_ntoa((ip4_addr_t *)&gw),
                  ip4addr_ntoa((ip4_addr_t *)&mask));
    }
}

static void event_cb_wifi_event(input_event_t *event, void *private_data)
{
    switch (event->code) {
    case CODE_WIFI_ON_INIT_DONE:
        blog_info("[NET] init done");
        wifi_mgmr_start_background(&conf);
        break;

    case CODE_WIFI_ON_MGMR_DONE:
        blog_info("[NET] mgmr done, connecting to %s", s_ssid);
        {
            wifi_interface_t iface;

            iface = wifi_mgmr_sta_enable();
            wifi_mgmr_sta_connect(iface, s_ssid, s_pwd, NULL, NULL, 0, 0);
        }
        break;

    case CODE_WIFI_ON_GOT_IP:
        blog_info("[NET] got ip");
        wifi_sta_ipv4_print();
        break;

    case CODE_WIFI_ON_DISCONNECT:
        blog_info("[NET] disconnected (%ld), autoconnect", event->value);
        break;

    default:
        break;
    }
}

static void proc_main_entry(void *pvParameters)
{
    (void)pvParameters;

    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
    hal_wifi_start_firmware_task();
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    vTaskDelete(NULL);
}

void wifi_sta_start(const char *ssid, const char *pwd)
{
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';
    strncpy(s_pwd, pwd, sizeof(s_pwd) - 1);
    s_pwd[sizeof(s_pwd) - 1] = '\0';

    tcpip_init(NULL, NULL);
    xTaskCreate(proc_main_entry, (char *)"wifi_sta", 1024, NULL, 15, NULL);
}