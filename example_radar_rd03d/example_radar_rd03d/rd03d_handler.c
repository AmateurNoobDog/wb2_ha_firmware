/**
 * @file rd03d_handler.c
 * @brief Home Assistant 设备回调实现与雷达数据缓存
 *
 * 缓存由雷达任务写入, TCP 任务在 get_state 中读取, 使用短临界区保护拷贝,
 * 保证一帧 30 字节数据不会被读到写了一半的状态。
 */
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "rd03d_handler.h"
#include "app_config.h"
#include "ha_json.h"
#include "ha_push.h"
#include <wifi_mgmr_ext.h>

/*--------------------------- 数据缓存 ---------------------------*/

static rd03d_frame_t s_frame;
static volatile int  s_frame_valid = 0;   /* 是否收到过至少一帧有效数据 */

void rd03d_handler_update(const rd03d_frame_t *frame)
{
    /* 雷达任务上下文: 短临界区拷贝整帧, 避免 get_state 读到半帧 */
    taskENTER_CRITICAL();
    s_frame = *frame;
    s_frame_valid = 1;
    taskEXIT_CRITICAL();
}

void rd03d_handler_get_frame(rd03d_frame_t *frame, int *valid)
{
    taskENTER_CRITICAL();
    *frame = s_frame;
    *valid = s_frame_valid;
    taskEXIT_CRITICAL();
}

void rd03d_gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%03d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], seq);
}

static void get_mac(uint8_t *mac)
{
    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, 6);
    }
}

/*------------------------ ha_device_t 回调 ------------------------*/

int rd03d_handler_get_device(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;
    int i;

    get_mac(mac);

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");

    /* 有人 */
    rd03d_gen_entity_id(id, sizeof(id), mac, RD03D_ENTITY_PRESENCE);
    used += snprintf(buf + used, buf_len - used,
        "{\"id\":\"%s\",\"type\":\"binary_sensor\",\"name\":\"有人\","
        "\"icon\":\"mdi:motion-sensor\"}", id);

    /* 每目标 X/Y/速度 3 个 sensor */
    for (i = 0; i < RD03D_RPT_TARGET_NUM; i++) {
        int base = RD03D_ENTITY_TARGET_BASE + i * 3;

        rd03d_gen_entity_id(id, sizeof(id), mac, base);
        used += snprintf(buf + used, buf_len - used,
            ",{\"id\":\"%s\",\"type\":\"sensor\",\"name\":\"目标%d X\","
            "\"icon\":\"mdi:axis-x-light\",\"unit\":\"mm\"}", id, i + 1);

        rd03d_gen_entity_id(id, sizeof(id), mac, base + 1);
        used += snprintf(buf + used, buf_len - used,
            ",{\"id\":\"%s\",\"type\":\"sensor\",\"name\":\"目标%d Y\","
            "\"icon\":\"mdi:axis-y-light\",\"unit\":\"mm\"}", id, i + 1);

        rd03d_gen_entity_id(id, sizeof(id), mac, base + 2);
        used += snprintf(buf + used, buf_len - used,
            ",{\"id\":\"%s\",\"type\":\"sensor\",\"name\":\"目标%d 速度\","
            "\"icon\":\"mdi:speedometer\",\"unit\":\"cm/s\"}", id, i + 1);
    }

    used += snprintf(buf + used, buf_len - used, "]");

    /* 不启用 push-only 模式(固定 0): 目标消失不推送, 必须由 HA 轮询才能看到 */
    used += snprintf(buf + used, buf_len - used,
        ",\"offline_timeout\":0");

    return used;
}

/* 追加一个 sensor 实体状态; show=0 时输出 null(目标不存在/尚未收到帧) */
static int append_sensor(char *buf, int buf_len, int used, const char *id,
                         int show, int value)
{
    if (show) {
        return used + snprintf(buf + used, buf_len - used,
            ",{\"id\":\"%s\",\"type\":\"sensor\",\"value\":%d}", id, value);
    }
    return used + snprintf(buf + used, buf_len - used,
        ",{\"id\":\"%s\",\"type\":\"sensor\",\"value\":null}", id);
}

int rd03d_handler_get_state(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    rd03d_frame_t frame;
    int valid = 0;
    int used = 0;
    int i;

    rd03d_handler_get_frame(&frame, &valid);
    get_mac(mac);

    /* 有人: 从未收到有效帧时按无人处理 */
    rd03d_gen_entity_id(id, sizeof(id), mac, RD03D_ENTITY_PRESENCE);
    used += snprintf(buf + used, buf_len - used,
        "\"entities\":[{\"id\":\"%s\",\"type\":\"binary_sensor\",\"value\":%d}",
        id, (valid && frame.any_target) ? 1 : 0);

    for (i = 0; i < RD03D_RPT_TARGET_NUM; i++) {
        const rd03d_target_t *t = &frame.targets[i];
        int base = RD03D_ENTITY_TARGET_BASE + i * 3;
        int show = valid && t->valid;

        rd03d_gen_entity_id(id, sizeof(id), mac, base);
        used = append_sensor(buf, buf_len, used, id, show, t->x_mm);

        rd03d_gen_entity_id(id, sizeof(id), mac, base + 1);
        used = append_sensor(buf, buf_len, used, id, show, t->y_mm);

        rd03d_gen_entity_id(id, sizeof(id), mac, base + 2);
        used = append_sensor(buf, buf_len, used, id, show, t->speed_cms);
    }

    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int rd03d_handler_set_state(const char *cmd_json)
{
    const char *cmd = ha_json_str(cmd_json, "cmd");

    if (cmd == NULL) {
        return -1;
    }

    /* 雷达本身无需控制命令, 仅保留推送目标配置 */
    if (strcmp(cmd, "push_cfg") == 0) {
        const char *host = ha_json_str(cmd_json, "host");

        if (host == NULL) {
            host = ha_json_str(cmd_json, "ip");
        }
        if (host != NULL) {
            int port = ha_json_int(cmd_json, "port", HA_PUSH_DEFAULT_PORT);

            ha_push_set_target(host, (uint16_t)port);
        }
        return 0;
    }

    return 0;
}
