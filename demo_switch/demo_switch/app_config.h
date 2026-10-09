#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define SWITCH_PINS        {3, 14, 17}
#define SWITCH_COUNT       3
#define RELAY_ACTIVE_HIGH  1
#define SWITCH_NAMES       {"开关" "1", "开关" "2", "开关" "3"}

#define TCP_SERVER_PORT    9100
#define TCP_SERVER_STACK   4096

#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "智能开关"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_MANUFACTURER "AND-DIY"
#define DEVICE_SW_VERSION  "1.1.0"

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

#define STORE_KEY_VOLUME      "TTS_VOLUME"
#define STORE_KEY_SPEED       "TTS_SPEED"
#define TTS_DEFAULT_VOLUME    5
#define TTS_DEFAULT_SPEED     5

#endif /* __APP_CONFIG_H__ */