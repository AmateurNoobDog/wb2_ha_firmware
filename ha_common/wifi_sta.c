#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <wifi_mgmr_ext.h>
#include <hal_wifi.h>

#include "wifi_sta.h"

static char s_ssid[64];
static char s_pwd[64];

static void wifi_sta_event_cb(input_event_t *event, void *private_data)
{
    switch (event->code) {
    case CODE_WIFI_ON_INIT_DONE:
        printf("[WIFI] init done\n");
        wifi_mgmr_start_background(&(wifi_conf_t){ .country_code = "CN" });
        break;
    case CODE_WIFI_ON_MGMR_DONE:
    {
        printf("[WIFI] mgmr done, connecting to %s\n", s_ssid);
        wifi_interface_t iface = wifi_mgmr_sta_enable();
        wifi_mgmr_sta_connect(iface, s_ssid, s_pwd, NULL, NULL, 0, 0);
        break;
    }
    case CODE_WIFI_ON_DISCONNECT:
        printf("[WIFI] disconnected\n");
        break;
    case CODE_WIFI_ON_GOT_IP:
        printf("[WIFI] got ip\n");
        break;
    default:
        break;
    }
}

void wifi_sta_start(const char *ssid, const char *pwd)
{
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';
    strncpy(s_pwd, pwd, sizeof(s_pwd) - 1);
    s_pwd[sizeof(s_pwd) - 1] = '\0';

    aos_register_event_filter(EV_WIFI, wifi_sta_event_cb, NULL);
    hal_wifi_start_firmware_task();
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
}
