#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

// Radar hardware configuration
#define RADAR_CS_PIN          22      // SPI chip select pin
#define RADAR_POWER_PIN       25      // Radar power control pin

// TCP server configuration
#define TCP_SERVER_PORT       9100
#define TCP_SERVER_STACK      4096

// Device identity
#define DEVICE_TYPE           "radar"
#define DEVICE_NAME           "雷达"
#define DEVICE_MODEL          "RD-01"
#define DEVICE_SW_VERSION     "1.0.0"

// Reboot provisioning detection
#define REBOOT_PROVISION_COUNT  3
#define STORE_KEY_BOOT_CNT      "BOOT_CNT"

// Storage keys
#define STORE_KEY_SSID        "ROUTER_SSID"
#define STORE_KEY_PWD         "ROUTER_PWD"
#define STORE_SSID_MAX        64
#define STORE_PWD_MAX         64

// Push reporting configuration
#define HA_PUSH_DEFAULT_PORT  9101    // HA push listening port (default)
#define STORE_KEY_HA_IP       "HA_IP"
#define STORE_KEY_HA_PORT     "HA_PORT"
#define STORE_IP_MAX          16      // "255.255.255.255" + \0

// Radar data processing configuration
#define FUNC_QUEUE_SIZE       34      // SPI data queue size
#define DATA_PROCESS_TASK_PRIORITY  14
#define CMD_PROCESS_TASK_PRIORITY   13
#define UART_RECV_CHECK_TASK_PRIORITY 12

// Radar debug switches
#define RADAR_GATE_DATA_ENABLE    0   // 门数据上报开关，1=开启，0=屏蔽
#define RADAR_DEBUG_COUNTER_ENABLE 0  // 调试计数器开关，1=开启，0=屏蔽

// 静止判定防抖：连续检测到几次无运动才判定为静止
#define STATIONARY_CONFIRM_COUNT   10

#endif /* __APP_CONFIG_H__ */
