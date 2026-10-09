#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define UART_DEBUG_TX_PIN   16
#define UART_DEBUG_RX_PIN   7
#define UART_DEBUG_BAUDRATE 115200

#define UART_TTS_TX_PIN     4
#define UART_TTS_RX_PIN     11
#define UART_TTS_BAUDRATE   9600

#define TCP_SERVER_PORT    9100
#define TCP_SERVER_STACK   4096

#define DEVICE_TYPE        "tts"
#define DEVICE_NAME        "\xE8\xAF\xAD\xE9\x9F\xB3\xE5\x90\x88\xE6\x88\x90"
#define DEVICE_MODEL       "Ai-WB2-TW-TTS"
#define DEVICE_MANUFACTURER "AND-DIY"
#define DEVICE_SW_VERSION  "1.1.0"

#define TTS_ENTITY_NOTIFY   "001"
#define TTS_ENTITY_VOLUME   "002"
#define TTS_ENTITY_SPEED    "003"

#define TTS_VOLUME_MIN      0
#define TTS_VOLUME_MAX      9
#define TTS_VOLUME_STEP     1

#define TTS_SPEED_MIN       0
#define TTS_SPEED_MAX       9
#define TTS_SPEED_STEP      1

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
