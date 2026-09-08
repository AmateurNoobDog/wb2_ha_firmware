#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lwip/tcpip.h>
#include <bl_sys.h>
#include <hal_sys.h>
#include <cli.h>

#include "wifi_interface.h"
#include <../wifi_mgmr.h>
#include "blufi.h"
#include "blufi_api.h"
#include "blufi_hal.h"
#include "blufi_init.h"
#include "axk_blufi.h"
#include "ble_interface.h"
#include "blufi_security.h"

#include "blufi_app.h"
#include "store.h"
#include "ha_push.h"
#include "ha_json.h"
#include "app_config.h"

static int scan_counter;
static bool ble_is_connected = false;
static bool gl_sta_connected = false;

blufi_config_t g_blufi_config = {0};

static void cb_scan_item_parse(wifi_mgmr_ap_item_t *env, uint32_t *param1, wifi_mgmr_ap_item_t *item)
{
    _blufi_ap_record_t *ap_list;

    ap_list = (_blufi_ap_record_t *)env;
    ap_list[scan_counter].rssi = item->rssi;
    memset(ap_list[scan_counter].ssid, 0, sizeof(ap_list[scan_counter].ssid));
    memcpy(ap_list[scan_counter].ssid, item->ssid, item->ssid_len);

    scan_counter++;
}

static void cb_scan_complete(void *data, void *param)
{
    _blufi_ap_record_t *ap_list;

    ap_list = (_blufi_ap_record_t *)malloc(WIFI_MGMR_SCAN_ITEMS_MAX * sizeof(_blufi_ap_record_t));
    if (!ap_list) {
        printf("[BLUFI] ap_list malloc fail\n");
        return;
    }

    scan_counter = 0;
    wifi_mgmr_scan_ap_all(ap_list, NULL, cb_scan_item_parse);
    printf("[BLUFI] scan complete:%ld\n", scan_counter);

    if (ble_is_connected == true) {
        axk_blufi_send_wifi_list(scan_counter, ap_list);
    } else {
        printf("[BLUFI] BLE is not connected yet\n");
    }

    free(ap_list);
}

static int wifi_scan_start(void)
{
    return wifi_mgmr_scan(NULL, cb_scan_complete);
}

static void blufi_wifi_event(int event, void *param)
{
    switch (event) {
    case BLUFI_STATION_CONNECTED:
        gl_sta_connected = true;
        printf("[BLUFI] station connected\n");
        break;

    case BLUFI_STATION_DISCONNECTED:
        gl_sta_connected = false;
        printf("[BLUFI] station disconnected\n");
        break;

    case BLUFI_STATION_GOT_IP:
    {
        axk_blufi_extra_info_t info;

        memset(&info, 0, sizeof(axk_blufi_extra_info_t));
        wifi_conn_ap_info_get(&g_blufi_config.wifi.sta);
        memcpy(info.sta_bssid, g_blufi_config.wifi.sta.cwjap_param.bssid, 6);
        info.sta_bssid_set = true;
        info.sta_ssid = (uint8_t *)g_blufi_config.wifi.sta.cwjap_param.ssid;
        info.sta_ssid_len = strlen(g_blufi_config.wifi.sta.cwjap_param.ssid);

        if (ble_is_connected == true) {
            axk_blufi_send_wifi_conn_report(g_blufi_config.wifi.cwmode,
                                            _BLUFI_STA_CONN_SUCCESS, 0, &info);
        } else {
            printf("[BLUFI] BLE is not connected yet\n");
        }

        printf("[BLUFI] provisioning ok, connected\n");
        g_blufi_config.wifi.cwmode = WIFIMODE_STA;
        store_reboot_provision_clear();
        vTaskDelay(pdMS_TO_TICKS(500));
        printf("[BLUFI] rebooting into normal mode\n");
        hal_sys_reset();
    }
    break;

    default:
        break;
    }
}

static void example_event_callback(_blufi_cb_event_t event, _blufi_cb_param_t *param)
{
    switch (event) {
    case AXK_BLUFI_EVENT_INIT_FINISH:
        printf("[BLUFI] init finish\n");
        axk_blufi_adv_start();
        break;

    case AXK_BLUFI_EVENT_DEINIT_FINISH:
        printf("[BLUFI] deinit finish\n");
        break;

    case AXK_BLUFI_EVENT_BLE_CONNECT:
        printf("[BLUFI] ble connect\n");
        ble_is_connected = true;
        axk_blufi_adv_stop();
        blufi_security_init();
        break;

    case AXK_BLUFI_EVENT_BLE_DISCONNECT:
        printf("[BLUFI] ble disconnect\n");
        ble_is_connected = false;
        blufi_security_deinit();
        axk_blufi_adv_start();
        break;

    case AXK_BLUFI_EVENT_SET_WIFI_OPMODE:
        printf("[BLUFI] set wifi opmode %d\n", param->wifi_mode.op_mode);
        g_blufi_config.wifi.cwmode = WIFIMODE_STA;
        break;

    case AXK_BLUFI_EVENT_REQ_CONNECT_TO_AP:
    {
        cwjap_param_t cwjap_param = {0};

        printf("[BLUFI] request wifi connect to AP\n");
        cwjap_param = g_blufi_config.wifi.sta.cwjap_param;
        if (axk_hal_conn_ap_info_set(&cwjap_param) != BLUFI_ERR_SUCCESS) {
            printf("[BLUFI] axk_hal_conn_ap_info_set fail\n");
            break;
        }
        g_blufi_config.wifi.sta.state = BLUFI_WIFI_STATE_CONNECTING;
    }
    break;

    case AXK_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        printf("[BLUFI] request disconnect from AP\n");
        axk_hal_disconn_ap();
        break;

    case AXK_BLUFI_EVENT_REPORT_ERROR:
        printf("[BLUFI] report error, error code %d\n", param->report_error.state);
        axk_blufi_send_error_info(param->report_error.state);
        break;

    case AXK_BLUFI_EVENT_GET_WIFI_STATUS:
    {
        wifi_mode_t mode = g_blufi_config.wifi.cwmode;

        if (gl_sta_connected) {
            axk_blufi_extra_info_t info;

            memset(&info, 0, sizeof(axk_blufi_extra_info_t));
            wifi_conn_ap_info_get(&g_blufi_config.wifi.sta);
            memcpy(info.sta_bssid, g_blufi_config.wifi.sta.cwjap_param.bssid, 6);
            info.sta_bssid_set = true;
            info.sta_ssid = (uint8_t *)g_blufi_config.wifi.sta.cwjap_param.ssid;
            info.sta_ssid_len = strlen(g_blufi_config.wifi.sta.cwjap_param.ssid);
            axk_blufi_send_wifi_conn_report(mode, _BLUFI_STA_CONN_SUCCESS, 0, &info);
        } else {
            axk_blufi_send_wifi_conn_report(mode, _BLUFI_STA_CONN_FAIL, 0, NULL);
        }
        printf("[BLUFI] get wifi status\n");
    }
    break;

    case AXK_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        printf("[BLUFI] close a gatt connection\n");
        axk_blufi_disconnect();
        break;

    case AXK_BLUFI_EVENT_DEAUTHENTICATE_STA:
        break;

    case AXK_BLUFI_EVENT_RECV_STA_BSSID:
        memset(g_blufi_config.wifi.sta.cwjap_param.bssid, 0, 6);
        memcpy(g_blufi_config.wifi.sta.cwjap_param.bssid, param->sta_bssid.bssid, 6);
        printf("[BLUFI] recv sta bssid\n");
        break;

    case AXK_BLUFI_EVENT_RECV_STA_SSID:
        memset(g_blufi_config.wifi.sta.cwjap_param.ssid, 0, 33);
        strncpy(g_blufi_config.wifi.sta.cwjap_param.ssid, (char *)param->sta_ssid.ssid,
                param->sta_ssid.ssid_len);
        printf("[BLUFI] recv sta ssid: %s\n", (char *)g_blufi_config.wifi.sta.cwjap_param.ssid);
        store_wifi_save_ssid(param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        break;

    case AXK_BLUFI_EVENT_RECV_STA_PASSWD:
        memset(g_blufi_config.wifi.sta.cwjap_param.pwd, 0, 64);
        strncpy(g_blufi_config.wifi.sta.cwjap_param.pwd, (char *)param->sta_passwd.passwd,
                param->sta_passwd.passwd_len);
        printf("[BLUFI] recv sta password (%d bytes)\n", param->sta_passwd.passwd_len);
        store_wifi_save_pwd(param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        break;

    case AXK_BLUFI_EVENT_RECV_SOFTAP_SSID:
    case AXK_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
    case AXK_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
    case AXK_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
    case AXK_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        break;

    case AXK_BLUFI_EVENT_GET_WIFI_LIST:
        wifi_scan_start();
        break;

    case AXK_BLUFI_EVENT_RECV_CUSTOM_DATA:
    {
        printf("[BLUFI] recv custom data len:%d\n", param->custom_data.data_len);
        const char *ha_ip = ha_json_str((const char *)param->custom_data.data, "ha_ip");
        if (ha_ip) {
            int ha_port = ha_json_int((const char *)param->custom_data.data, "ha_port",
                                      HA_PUSH_DEFAULT_PORT);
            ha_push_set_target(ha_ip, (uint16_t)ha_port);
            printf("[BLUFI] push target set: %s:%d\n", ha_ip, ha_port);
        }
        axk_blufi_send_custom_data(param->custom_data.data, param->custom_data.data_len);
        break;
    }

    case AXK_BLUFI_EVENT_RECV_USERNAME:
    case AXK_BLUFI_EVENT_RECV_CA_CERT:
    case AXK_BLUFI_EVENT_RECV_CLIENT_CERT:
    case AXK_BLUFI_EVENT_RECV_SERVER_CERT:
    case AXK_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
    case AXK_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        break;

    default:
        break;
    }
}

static _blufi_callbacks_t example_callbacks = {
    .event_cb = example_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

static int at_blufi_start(void)
{
    int ret = -1;

    axk_hal_blufi_init();
    ret = _blufi_host_and_cb_init(&example_callbacks);
    if (ret) {
        printf("[BLUFI] host_and_cb_init failed: %d\n", ret);
    }
    return ret;
}

static void cmd_blufi_init(char *buf, int len, int argc, char **argv)
{
    at_blufi_start();
}

static void cmd_blufi_deinit(char *buf, int len, int argc, char **argv)
{
    axk_blufi_profile_deinit();
    axk_hal_blufi_deinit();
    axk_blufi_adv_stop();
    axk_hal_ble_role_set(BLE_ROLE_DEINIT);
}

const static struct cli_command cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"blufi_init", "blufi init", cmd_blufi_init},
    {"blufi_deinit", "blufi deinit", cmd_blufi_deinit},
};

static void proc_main_entry(void *pvParameters)
{
    wifi_interface_init(blufi_wifi_event);
    at_blufi_start();
    vTaskDelete(NULL);
}

void blufi_app_start(void)
{
    bl_sys_init();
    xTaskCreate(proc_main_entry, (char *)"blufi_task", 1024, NULL, 15, NULL);
    tcpip_init(NULL, NULL);
    printf("[BLUFI] provisioning started\n");
}
