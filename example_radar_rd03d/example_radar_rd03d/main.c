/**
 * @file main.c
 * @brief example_radar_rd03d — Rd-03D_V2 毫米波雷达 Home Assistant 固件
 *
 * 硬件连接(默认, 见 app_config.h 宏):
 *   雷达 UART1: TX=GPIO4 -> 雷达 RX, RX=GPIO5 <- 雷达 TX, 256000 8N1
 *   日志 UART0: TX=GPIO16, RX=GPIO7, 115200 8N1 (系统 console, 本工程不可占用)
 *
 * 流程:
 *   开机 → BOOT_CNT++ → 有 WiFi 配置则连接, 否则/连刷 3 次进 BluFi 配网
 *   got_ip → ha_push_init + mDNS 注册 + 启动 TCP JSON 服务器(端口 9100)
 *   雷达任务(与网络无关, 上电即运行):
 *     初始化雷达串口 → 下发配置(使能/多目标/版本/结束) → 循环解析上报帧
 *     → 更新缓存供 HA 轮询 → 按速度自适应节流推送给 HA
 *
 * 推送节流(每目标独立一条消息):
 *   interval = clamp(PUSH_MAX_MS/(1+|speed|), PUSH_MIN_MS, PUSH_MAX_MS)
 *   目标消失不推送, 由 HA 轮询 get_state 更新。
 */
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aos/yloop.h>

#include <lwip/tcpip.h>
#include "uart_app.h"
#include "store.h"
#include "wifi_sta.h"
#include "blufi_app.h"
#include "ha_device.h"
#include "ha_push.h"
#include "ha_mdns.h"
#include "rd03d.h"
#include "rd03d_handler.h"
#include "app_config.h"
#include <wifi_mgmr_ext.h>

static volatile int s_net_ready = 0;

/* 雷达串口由 uart_app 的 uart_1 承载(extern 见 uart_app.h), 本文件不再自建设备 */

/**
 * @brief 开机自动下发配置命令(使能配置 -> 多目标模式 -> 版本号 -> 结束配置)
 */
static void rd03d_init_config(hosal_uart_dev_t *uart)
{
    char version[32] = { 0 };
    int  ret;

    ret = rd03d_enable_config(uart);
    if (ret != 0) {
        uart_logf("[RD03D] enable config failed (%d), check wiring/baudrate", ret);
        return;
    }
    uart_log("[RD03D] enable config OK");

    ret = rd03d_set_mode(uart, RD03D_MODE_MULTI);
    uart_logf("[RD03D] set multi-target mode: %s", (ret == 0) ? "OK" : "FAIL");

    ret = rd03d_query_version(uart, version, sizeof(version));
    if (ret == 0) {
        uart_logf("[RD03D] version: %s", version);
    } else {
        uart_logf("[RD03D] query version: FAIL (%d)", ret);
    }

    ret = rd03d_end_config(uart);
    uart_logf("[RD03D] end config: %s (radar start reporting)", (ret == 0) ? "OK" : "FAIL");
}

/*--------------------------- 推送节流 ---------------------------*/

/* 每目标上次推送时间与数值 */
static TickType_t s_last_push[RD03D_RPT_TARGET_NUM];
static uint8_t    s_pushed_flag[RD03D_RPT_TARGET_NUM];   /* 0=从未推送过 */
static int16_t    s_pushed_x[RD03D_RPT_TARGET_NUM];
static int16_t    s_pushed_y[RD03D_RPT_TARGET_NUM];
static int16_t    s_pushed_speed[RD03D_RPT_TARGET_NUM];
/* 有人状态: 0xFF 表示从未推送 */
static uint8_t    s_pushed_presence = 0xFF;

/**
 * @brief 按目标速度计算推送节流周期
 * interval = clamp(PUSH_MAX_MS/(1+|speed|), PUSH_MIN_MS, PUSH_MAX_MS)
 */
static TickType_t push_interval_ticks(int16_t speed_cms)
{
    int speed_abs = (speed_cms < 0) ? -speed_cms : speed_cms;
    int interval  = PUSH_MAX_MS / (1 + speed_abs);

    if (interval > PUSH_MAX_MS) {
        interval = PUSH_MAX_MS;
    }
    if (interval < PUSH_MIN_MS) {
        interval = PUSH_MIN_MS;
    }
    return pdMS_TO_TICKS(interval);
}

/**
 * @brief 新一帧到达时按节流规则推送
 *
 * - 有人状态变化 → 立即推送单实体
 * - 目标首次出现/数值变化且到达节流周期 → 推送该目标 X/Y/速度(一条消息)
 * - 目标消失 → 不推送, 置"未推送"标志, 下次出现按首次处理
 */
static void radar_on_frame(const rd03d_frame_t *frame)
{
    uint8_t presence;
    char json[256];
    int n;
    int i;

    if (!s_net_ready || !ha_push_enabled()) {
        return;
    }

    /* 有人状态 */
    presence = frame->any_target ? 1 : 0;
    if (presence != s_pushed_presence) {
        uint8_t mac[6];
        char id[20];

        if (wifi_mgmr_sta_mac_get(mac) != 0) {
            memset(mac, 0, sizeof(mac));
        }
        rd03d_gen_entity_id(id, sizeof(id), mac, RD03D_ENTITY_PRESENCE);

        n = snprintf(json, sizeof(json),
                     "{\"id\":\"%s\",\"type\":\"binary_sensor\",\"value\":%d}",
                     id, presence);
        if (n > 0 && n < (int)sizeof(json)) {
            ha_push_send(json);
            s_pushed_presence = presence;
        }
    }

    /* 各目标 X/Y/速度 */
    for (i = 0; i < RD03D_RPT_TARGET_NUM; i++) {
        const rd03d_target_t *t = &frame->targets[i];
        uint8_t mac[6];
        char id_x[20], id_y[20], id_v[20];
        int base;
        int changed;
        int first;
        int m;

        if (!t->valid) {
            /* 目标消失: 不推送, 下次出现按首次处理(立即推) */
            s_pushed_flag[i] = 0;
            continue;
        }

        first = (s_pushed_flag[i] == 0);
        changed = first ||
                  (t->x_mm      != s_pushed_x[i]) ||
                  (t->y_mm      != s_pushed_y[i]) ||
                  (t->speed_cms != s_pushed_speed[i]);
        if (!changed) {
            continue;
        }
        if (!first &&
            (xTaskGetTickCount() - s_last_push[i]) < push_interval_ticks(t->speed_cms)) {
            continue;   /* 未到节流周期 */
        }

        if (wifi_mgmr_sta_mac_get(mac) != 0) {
            memset(mac, 0, sizeof(mac));
        }
        base = RD03D_ENTITY_TARGET_BASE + i * 3;
        rd03d_gen_entity_id(id_x, sizeof(id_x), mac, base);
        rd03d_gen_entity_id(id_y, sizeof(id_y), mac, base + 1);
        rd03d_gen_entity_id(id_v, sizeof(id_v), mac, base + 2);

        m = snprintf(json, sizeof(json),
                     "{\"entities\":["
                     "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%d},"
                     "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%d},"
                     "{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%d}"
                     "]}",
                     id_x, t->x_mm, id_y, t->y_mm, id_v, t->speed_cms);
        if (m > 0 && m < (int)sizeof(json)) {
            ha_push_send(json);
            s_last_push[i]     = xTaskGetTickCount();
            s_pushed_flag[i]   = 1;
            s_pushed_x[i]      = t->x_mm;
            s_pushed_y[i]      = t->y_mm;
            s_pushed_speed[i]  = t->speed_cms;
        }
    }
}

/*--------------------------- 雷达任务 ---------------------------*/

#if RADAR_LOG_PERIOD_MS > 0
/**
 * @brief 节流打印最新目标数据, 每行格式: 序号\tx\ty\t速度 (mm / mm / cm/s)
 */
static void rd03d_print_frame(const rd03d_frame_t *frame, bool frame_received)
{
    uint8_t i;

    if (!frame_received) {
        printf("[RD03D] waiting for radar data... (no valid frame parsed yet)\r\n");
        return;
    }
    for (i = 0; i < RD03D_RPT_TARGET_NUM; i++) {
        const rd03d_target_t *t = &frame->targets[i];

        if (t->valid) {
            printf("%d\t%d\t%d\t%d\r\n", i + 1, t->x_mm, t->y_mm, t->speed_cms);
        } else {
            printf("%d\tnull\r\n", i + 1);
        }
    }
}
#endif

static void radar_task(void *param)
{
    static rd03d_parser_t parser;
    rd03d_frame_t latest;
    uint8_t  rx_buf[RADAR_RX_BUF_SIZE];   /* 必须 >= 128, 见 hosal_uart_receive 说明 */
    int      ret;
    bool     frame_received = false;
    TickType_t last_log = 0;

    (void)param;
    memset(&parser, 0, sizeof(parser));
    memset(&latest, 0, sizeof(latest));

    /* 串口在 main() 中由 uart_init() 统一初始化(uart0 日志口 + uart1 雷达口) */
    uart_logf("[RD03D] radar task start, uart%d tx=%d rx=%d baud=%d",
              RD03D_UART_ID, RD03D_UART_TX_PIN, RD03D_UART_RX_PIN, RD03D_UART_BAUDRATE);

    /* 等待雷达上电就绪 */
    vTaskDelay(pdMS_TO_TICKS(RD03D_POWERON_DELAY_MS));

    rd03d_init_config(&uart_1);

    last_log = xTaskGetTickCount();
    while (1) {
        /* 接收缓冲必须 >= 128, 见 hosal_uart_receive 说明 */
        ret = uart1_get(rx_buf, RADAR_RX_BUF_SIZE);
        if (ret > 0) {
            int i;

            for (i = 0; i < ret; i++) {
                rd03d_frame_t frame;

                if (rd03d_parser_input(&parser, rx_buf[i], &frame)) {
                    latest = frame;
                    frame_received = true;

                    rd03d_handler_update(&frame);
                    radar_on_frame(&frame);
                }
            }
        } else {
            vTaskDelay(1);
        }

#if RADAR_LOG_PERIOD_MS > 0
        if ((xTaskGetTickCount() - last_log) >= pdMS_TO_TICKS(RADAR_LOG_PERIOD_MS)) {
            last_log = xTaskGetTickCount();
            rd03d_print_frame(&latest, frame_received);
        }
#else
        (void)last_log;
        (void)frame_received;
#endif
    }
}

/*--------------------------- HA 服务 ---------------------------*/

static const ha_device_t ha_dev = {
    .type         = DEVICE_TYPE,
    .name         = DEVICE_NAME,
    .model        = DEVICE_MODEL,
    .manufacturer = DEVICE_MANUFACTURER,
    .sw_version   = DEVICE_SW_VERSION,
    .port         = TCP_SERVER_PORT,
    .get_device   = rd03d_handler_get_device,
    .get_state    = rd03d_handler_get_state,
    .set_state    = rd03d_handler_set_state,
    .log          = uart_log,     /* tcp_json_server 日志走 uart0 直写 */
};

static void on_got_ip(void)
{
    store_reboot_provision_clear();
    uart_log("[APP] got ip, starting tcp json server");
    uart_logf("[SYS] Memory left is %d Bytes", xPortGetFreeHeapSize());

    ha_push_init();
    ha_mdns_start();
    s_net_ready = 1;
    xTaskCreate(ha_tcp_server_start, (char *)"tcp_json", TCP_SERVER_STACK,
                (void *)&ha_dev, 15, NULL);
}

static void app_evt_cb(input_event_t *event, void *private_data)
{
    (void)private_data;

    switch (event->code) {
    case CODE_WIFI_ON_GOT_IP:
        on_got_ip();
        break;

    default:
        break;
    }
}

static void boot_mode(void)
{
    store_wifi_t cfg;

    store_init();
    aos_register_event_filter(EV_WIFI, app_evt_cb, NULL);

    if (store_reboot_provision_check(REBOOT_PROVISION_COUNT)) {
        uart_logf("[APP] %d consecutive reboots, enter provisioning mode",
                  REBOOT_PROVISION_COUNT);
        store_reboot_provision_clear();
        blufi_app_start();
        return;
    }

    if (store_wifi_load(&cfg)) {
        uart_logf("[APP] wifi ssid: %s", cfg.ssid);
        uart_log("[APP] connecting wifi");
        wifi_sta_start(cfg.ssid, cfg.pwd);
    } else {
        uart_log("[APP] no wifi config, enter blufi provisioning");
        store_reboot_provision_clear();
        blufi_app_start();
    }
}

void main(void)
{
    uart_init();    /* uart0 日志口(16/7@115200) + uart1 雷达口(4/5@256000) */
    uart_log("[OS] example_radar_rd03d starting...");
    tcpip_init(NULL, NULL);
    boot_mode();

    /* 雷达解析不依赖网络, 上电即运行 */
    xTaskCreate(radar_task, (char *)"radar", RADAR_TASK_STACK, NULL,
                RADAR_TASK_PRIORITY, NULL);

    uart_log("[OS] main exit");
}
