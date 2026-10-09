#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

/*-------------------------- 雷达硬件配置 --------------------------*/
/*
 * Rd-03D_V2 使用 UART1 控制器, 引脚 GPIO4/GPIO5:
 *   WB2 GPIO4 (TX) -> 雷达 RXD
 *   WB2 GPIO5 (RX) <- 雷达 TXD
 *
 * 注意: printf()/blog 日志硬绑定在 UART0 控制器上, 系统启动时已将其绑定到
 * GPIO16(TX)/GPIO7(RX) @115200 作为日志口。雷达必须避开 UART0, 否则 printf
 * 输出会以雷达波特率灌入雷达 RX, 日志同时失效。
 */
#define RD03D_UART_ID           1       /* 雷达接 UART1 控制器 */
#define RD03D_UART_TX_PIN       4       /* WB2 TX -> 雷达 RX */
#define RD03D_UART_RX_PIN       5       /* WB2 RX <- 雷达 TX */
#define RD03D_UART_BAUDRATE     256000  /* 雷达默认波特率 */

/* 调试日志口(uart0): 必须与系统 console 同引脚同波特率, 重复 init 才无害 */
#define UART_DEBUG_TX_PIN       16
#define UART_DEBUG_RX_PIN       7
#define UART_DEBUG_BAUDRATE     115200

/* 雷达上电到就绪的等待时间 */
#define RD03D_POWERON_DELAY_MS  2000

/*------------------------ 目标推送节流配置 ------------------------*/
/*
 * 每个目标独立一条推送消息, 节流周期:
 *   interval = clamp(PUSH_MAX_MS / (1 + |speed_cms|), PUSH_MIN_MS, PUSH_MAX_MS)
 * 速度越快推送越频繁(最短 PUSH_MIN_MS), 静止目标最慢 PUSH_MAX_MS 推一次。
 * 目标消失不推送, 由 HA 轮询 get_state 更新。
 */
#define PUSH_MAX_MS             5000    /* 静止/最慢节流周期 */
#define PUSH_MIN_MS             250     /* 高速目标最短节流周期 */

/* 解析结果调试打印周期, 0 = 关闭 */
#define RADAR_LOG_PERIOD_MS     0

/*--------------------------- TCP 服务 ---------------------------*/
#define TCP_SERVER_PORT         9100
#define TCP_SERVER_STACK        4096

/* 雷达接收任务 */
#define RADAR_TASK_STACK        2048
#define RADAR_TASK_PRIORITY     14
/* 接收缓冲必须 >= 128, 见 hosal_uart_receive 说明 */
#define RADAR_RX_BUF_SIZE       128

/*-------------------------- 设备身份 --------------------------*/
#define DEVICE_TYPE             "radar"
#define DEVICE_NAME             "毫米波雷达"
#define DEVICE_MODEL            "Rd-03D_V2"
#define DEVICE_MANUFACTURER     "Ai-Thinker"
#define DEVICE_SW_VERSION       "1.1.0"

/*--------------------- WiFi 配网 / 持久化 ---------------------*/
#define STORE_KEY_SSID          "ROUTER_SSID"
#define STORE_KEY_PWD           "ROUTER_PWD"
#define STORE_SSID_MAX          64
#define STORE_PWD_MAX           64

#define REBOOT_PROVISION_COUNT  3
#define STORE_KEY_BOOT_CNT      "BOOT_CNT"

/* 推送目标配置 */
#define HA_PUSH_DEFAULT_PORT    9101
#define STORE_KEY_HA_IP         "HA_IP"
#define STORE_KEY_HA_PORT       "HA_PORT"
#define STORE_IP_MAX            16

/* store.h 依赖的 TTS 键(本工程不使用) */
#define STORE_KEY_VOLUME        "TTS_VOLUME"
#define STORE_KEY_SPEED         "TTS_SPEED"
#define TTS_DEFAULT_VOLUME      5
#define TTS_DEFAULT_SPEED       5

#endif /* __APP_CONFIG_H__ */
