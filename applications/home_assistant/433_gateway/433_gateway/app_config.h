#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

/* UART 433 module configuration */
#define UART_433_ID          1
#define UART_433_TX_PIN      6
#define UART_433_RX_PIN      4
#define UART_433_BAUD        9600

/* GPIO for pair/reset control */
#define PAIR_IO              5

/* TCP server configuration */
#define TCP_SERVER_PORT      9100
#define TCP_SERVER_STACK     4096

/* Device information */
#define DEVICE_TYPE          "event"
#define DEVICE_NAME          "433\xE7\xBD\x91\xE5\x85\xB3"
#define DEVICE_MODEL         "Ai-WB2-12F"
#define DEVICE_SW_VERSION    "0.10.0"

/* Store keys */
#define STORE_KEY_SSID       "ROUTER_SSID"
#define STORE_KEY_PWD        "ROUTER_PWD"
#define STORE_SSID_MAX       64
#define STORE_PWD_MAX        64

/* Reboot provision */
#define REBOOT_PROVISION_COUNT  3
#define STORE_KEY_BOOT_CNT      "BOOT_CNT"

/* Push configuration */
#define HA_PUSH_DEFAULT_PORT  9101
#define STORE_KEY_HA_IP       "HA_IP"
#define STORE_KEY_HA_PORT     "HA_PORT"
#define STORE_IP_MAX          16

#endif /* __APP_CONFIG_H__ */
