#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define SWITCH_PINS        {3, 14, 17}
#define SWITCH_COUNT       3
#define RELAY_ACTIVE_HIGH  1
#define SWITCH_NAMES       {"\xE5\xBC\x80\xE5\x85\xB3" "1", "\xE5\xBC\x80\xE5\x85\xB3" "2", "\xE5\xBC\x80\xE5\x85\xB3" "3"}

#define TCP_SERVER_PORT    9100
#define TCP_SERVER_STACK   4096

#define DEVICE_TYPE        "sw"
#define DEVICE_NAME        "\xE6\x99\xBA\xE8\x83\xBD\xE5\xBC\x80\xE5\x85\xB3"
#define DEVICE_MODEL       "Ai-WB2-12F"

#define STORE_KEY_SSID     "ROUTER_SSID"
#define STORE_KEY_PWD      "ROUTER_PWD"
#define STORE_SSID_MAX     64
#define STORE_PWD_MAX      64

#endif /* __APP_CONFIG_H__ */