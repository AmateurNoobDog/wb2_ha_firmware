#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

// I2C configuration for DHT20
#define DHT20_I2C_SCL_PIN       12
#define DHT20_I2C_SDA_PIN       3
#define DHT20_I2C_FREQ          100000

// Sensor read interval
#define SENSOR_READ_INTERVAL_MS  5000

// TCP server
#define TCP_SERVER_PORT    9100
#define TCP_SERVER_STACK   4096

// Device identity
#define DEVICE_TYPE         "sensor"
#define DEVICE_NAME         "温湿度传感器"
#define DEVICE_MODEL        "Ai-WB2/DHT20"
#define DEVICE_MANUFACTURER "AND-DIY"
#define DEVICE_SW_VERSION   "1.1.0"

// WiFi provisioning
#define STORE_KEY_SSID     "ROUTER_SSID"
#define STORE_KEY_PWD      "ROUTER_PWD"
#define STORE_SSID_MAX     64
#define STORE_PWD_MAX      64

#define REBOOT_PROVISION_COUNT  3
#define STORE_KEY_BOOT_CNT      "BOOT_CNT"

// Push config
#define HA_PUSH_DEFAULT_PORT  9101
#define STORE_KEY_HA_IP       "HA_IP"
#define STORE_KEY_HA_PORT     "HA_PORT"
#define STORE_IP_MAX          16

// TTS (unused but needed by store.h)
#define STORE_KEY_VOLUME      "TTS_VOLUME"
#define STORE_KEY_SPEED       "TTS_SPEED"
#define TTS_DEFAULT_VOLUME    5
#define TTS_DEFAULT_SPEED     5

#endif /* __APP_CONFIG_H__ */
