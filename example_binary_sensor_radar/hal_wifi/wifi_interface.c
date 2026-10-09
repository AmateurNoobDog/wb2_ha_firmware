#include "wifi_interface.h"

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include <bl_wifi.h>
#include <hal_wifi.h>
#include <wifi_mgmr_ext.h>
#include <bl60x_fw_api.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <bl_timer.h>
#include <hal_sys.h>

extern void axk_radar_set_mac(const char *mac);

#define STA_DEFAULT_WIFI_SSID  "mqtt_test"
#define STA_DEFAULT_WIFI_PWD   "12345678"

uint8_t connect_flag = 0;

static char s_wifi_ssid[64] = STA_DEFAULT_WIFI_SSID;
static char s_wifi_pwd[64] = STA_DEFAULT_WIFI_PWD;

static wifi_conf_t conf =
{
    .country_code = "CN",
};

static void wifi_sta_connect(char *ssid, char *password)
{
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
}

static void axk_wifi_ap_start(void)
{
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_ap_enable();
    wifi_mgmr_ap_start(wifi_interface, "radar_test", 0, "12345678", 6);
}

static void event_cb_wifi_event(input_event_t *event, void *private_data)
{
    static char *ssid;
    static char *password;

    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            printf("[APP] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            wifi_mgmr_start_background(&conf);
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            printf("[APP] [EVT] MGMR DONE %lld, now %lums\r\n", aos_now_ms(), bl_timer_now_us()/1000);
            printf("[APP] [EVT] connecting to SSID: %s\r\n", s_wifi_ssid);
            wifi_sta_connect(s_wifi_ssid, s_wifi_pwd);
        }
        break;
        case CODE_WIFI_ON_MGMR_DENOISE:
        {
            printf("[APP] [EVT] Microwave Denoise is ON %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {
            printf("[APP] [EVT] SCAN Done %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE_ONJOIN:
        {
            printf("[APP] [EVT] SCAN On Join %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            printf("[APP] [EVT] disconnect %lld, Reason: %s\r\n",
                aos_now_ms(),
                wifi_mgmr_status_code_str(event->value)
            );
        }
        break;
        case CODE_WIFI_ON_CONNECTING:
        {
            printf("[APP] [EVT] Connecting %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_CMD_RECONNECT:
        {
            printf("[APP] [EVT] Reconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PRE_GOT_IP:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {
            printf("[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
            printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
            connect_flag = 1;

            uint8_t mac[6] = {0};
            if (bl_wifi_mac_addr_get(mac) == 0) {
                char mac_str[18] = {0};
                snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                axk_radar_set_mac(mac_str);
                printf("[APP] MAC: %s\r\n", mac_str);
            }
        }
        break;
        case CODE_WIFI_ON_EMERGENCY_MAC:
        {
            printf("[APP] [EVT] EMERGENCY MAC %lld\r\n", aos_now_ms());
            hal_reboot();//one way of handling emergency is reboot. Maybe we should also consider solutions
        }
        break;
        case CODE_WIFI_ON_PROV_SSID:
        {
            printf("[APP] [EVT] [PROV] [SSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
            if (ssid) {
                vPortFree(ssid);
                ssid = NULL;
            }
            ssid = (char*)event->value;
        }
        break;
        case CODE_WIFI_ON_PROV_BSSID:
        {
            printf("[APP] [EVT] [PROV] [BSSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
            if (event->value) {
                vPortFree((void*)event->value);
            }
        }
        break;
        case CODE_WIFI_ON_PROV_PASSWD:
        {
            printf("[APP] [EVT] [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
            if (password) {
                vPortFree(password);
                password = NULL;
            }
            password = (char*)event->value;
        }
        break;
        case CODE_WIFI_ON_PROV_CONNECT:
        {
            printf("[APP] [EVT] [PROV] [CONNECT] %lld\r\n", aos_now_ms());
            printf("connecting to %s:%s...\r\n", ssid, password);
            // wifi_sta_connect(ssid, password);
        }
        break;
        case CODE_WIFI_ON_PROV_DISCONNECT:
        {
            printf("[APP] [EVT] [PROV] [DISCONNECT] %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_AP_STA_ADD:
        {
            printf("[APP] [EVT] [AP] [ADD] %lld, sta idx is %lu\r\n", aos_now_ms(), (uint32_t)event->value);
            extern TaskHandle_t axk_tcp_task_handle;
            if (axk_tcp_task_handle != NULL)
            {
                xTaskNotifyGive(axk_tcp_task_handle);
            }
        }
        break;
        case CODE_WIFI_ON_AP_STA_DEL:
        {
            printf("[APP] [EVT] [AP] [DEL] %lld, sta idx is %lu\r\n", aos_now_ms(), (uint32_t)event->value);
        }
        break;
        default:
        {
            printf("[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}


void wifi_interface_init(void)
{
    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);

    hal_wifi_start_firmware_task();
    /*Trigger to start Wi-Fi*/
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
}

void wifi_set_credentials(const char *ssid, const char *pwd)
{
    if (ssid) {
        strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
        s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0';
    }
    if (pwd) {
        strncpy(s_wifi_pwd, pwd, sizeof(s_wifi_pwd) - 1);
        s_wifi_pwd[sizeof(s_wifi_pwd) - 1] = '\0';
    }
    printf("[WIFI] credentials set: SSID=%s\r\n", s_wifi_ssid);
}
