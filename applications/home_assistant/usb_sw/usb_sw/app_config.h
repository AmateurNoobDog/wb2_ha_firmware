#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define SWITCH_PINS        {4}
#define SWITCH_COUNT       1
#define RELAY_ACTIVE_HIGH  0
#define SWITCH_NAMES       {"USB"}

#define TCP_SERVER_PORT    9100
#define TCP_SERVER_STACK   4096

#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "USB通断器"
#define DEVICE_MODEL       "Ai-WB2-01S"
#define DEVICE_SW_VERSION  "1.0.0"

#define STORE_KEY_SSID     "ROUTER_SSID"
#define STORE_KEY_PWD      "ROUTER_PWD"
#define STORE_SSID_MAX     64
#define STORE_PWD_MAX      64

#define REBOOT_PROVISION_COUNT  3
#define STORE_KEY_BOOT_CNT      "BOOT_CNT"

#define HA_PUSH_DEFAULT_PORT  9101
#define STORE_KEY_HA_IP       "HA_IP"
#define STORE_KEY_HA_PORT     "HA_PORT"
#define STORE_IP_MAX          16

#endif /* __APP_CONFIG_H__ */
