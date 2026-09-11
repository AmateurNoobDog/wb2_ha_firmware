#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "radar_handler.h"
#include "app_config.h"
#include "body_presence.h"
#include "ha_json.h"
#include "ha_push.h"
#include "cmdprocess.h"
#include "banyan.h"
#include "AlgorithmConfig.h"
#include <wifi_mgmr_ext.h>

static uint8_t g_radar_motion = 0;
static uint8_t g_stationary_count = 0;  // 连续静止计数
static uint32_t g_state_call_count = 0;
static uint32_t g_motion_call_count = 0;

void gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq)
{
    snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X_%03d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], seq);
}

void radar_handler_init(void)
{
}

int radar_handler_get_device(char *buf, int buf_len)
{
    uint8_t mac[6];
    char id[20];
    int used = 0;

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }

    used += snprintf(buf + used, buf_len - used, "\"entities\":[");

    gen_entity_id(id, sizeof(id), mac, 1);
    used += snprintf(buf + used, buf_len - used,
                     "{\"id\":\"%s\",\"type\":\"binary_sensor\",\"name\":\"有人\","
                     "\"icon\":\"mdi:motion-sensor\"}", id);

    gen_entity_id(id, sizeof(id), mac, 2);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"binary_sensor\",\"name\":\"运动\","
                     "\"icon\":\"mdi:run-fast\"}", id);

    gen_entity_id(id, sizeof(id), mac, 3);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"button\",\"name\":\"标定无人\","
                     "\"icon\":\"mdi:cog-counterclockwise\",\"action\":\"calibrate\"}", id);

    gen_entity_id(id, sizeof(id), mac, 4);
    used += snprintf(buf + used, buf_len - used,
                     ",{\"id\":\"%s\",\"type\":\"button\",\"name\":\"恢复默认参数\","
                     "\"icon\":\"mdi:restore\",\"action\":\"restore\"}", id);

    used += snprintf(buf + used, buf_len - used, "]");
    return used;
}

int radar_handler_get_state(char *buf, int buf_len)
{
    g_state_call_count++;
    uint8_t mac[6];
    char id1[20], id2[20];

    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    gen_entity_id(id1, sizeof(id1), mac, 1);
    gen_entity_id(id2, sizeof(id2), mac, 2);

    int presence = (g_radar_motion >= 1 && g_radar_motion <= 3) ? 1 : 0;
    int motion = radar_handler_get_motion();  // 使用去抖后的 motion

    return snprintf(buf, buf_len,
                    "\"entities\":["
                    "{\"id\":\"%s\",\"type\":\"binary_sensor\",\"value\":%d},"
                    "{\"id\":\"%s\",\"type\":\"binary_sensor\",\"value\":%d}"
                    "]",
                    id1, presence, id2, motion);
}

int radar_handler_calibrate(void)
{
    uint32_t sum[9] = {0};
    uint32_t avg, threshold;
    int bin;

    printf("[CAL] start calibrate, sampling 10 times...\r\n");

    for (int i = 0; i < 10; i++) {
        for (bin = 1; bin <= 8; bin++) {
            sum[bin] += NormalDopplerMaxVal[bin];
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for (bin = 1; bin <= 8; bin++) {
        avg = sum[bin] / 10;
        threshold = avg * 2 + 5;
        gAlgorithmParam.nThresholdValOfMotion[bin] = threshold;
        printf("[CAL] bin%d: avg=%lu threshold=%lu\r\n", bin, avg, threshold);
    }

    Algo_SaveParameter((uint32_t *)&gAlgorithmParam, sizeof(ALGORITHM_PARAM_T));
    printf("[CAL] calibrate done, saved to flash\r\n");
    return 0;
}

int radar_handler_restore_defaults(void)
{
    printf("[CAL] restoring default parameters...\r\n");

    gAlgorithmParam.nMaxMotionRangeBin = PARAM_MOTION_MAX;
    gAlgorithmParam.nMaxMotionLessRangeBin = PARAM_MOTIONLESS_MAX;

    gAlgorithmParam.nThresholdValOfMotion[0] = PARAM_MOTION_SENSITIBITY_RANG0;
    gAlgorithmParam.nThresholdValOfMotion[1] = PARAM_MOTION_SENSITIBITY_RANG1;
    gAlgorithmParam.nThresholdValOfMotion[2] = PARAM_MOTION_SENSITIBITY_RANG2;
    gAlgorithmParam.nThresholdValOfMotion[3] = PARAM_MOTION_SENSITIBITY_RANG3;
    gAlgorithmParam.nThresholdValOfMotion[4] = PARAM_MOTION_SENSITIBITY_RANG4;
    gAlgorithmParam.nThresholdValOfMotion[5] = PARAM_MOTION_SENSITIBITY_RANG5;
    gAlgorithmParam.nThresholdValOfMotion[6] = PARAM_MOTION_SENSITIBITY_RANG6;
    gAlgorithmParam.nThresholdValOfMotion[7] = PARAM_MOTION_SENSITIBITY_RANG7;
    gAlgorithmParam.nThresholdValOfMotion[8] = PARAM_MOTION_SENSITIBITY_RANG8;

    gAlgorithmParam.nThresholdValOfMotionLess[0] = PARAM_MOTIONLESS_SENSITIBITY_RANG0;
    gAlgorithmParam.nThresholdValOfMotionLess[1] = PARAM_MOTIONLESS_SENSITIBITY_RANG1;
    gAlgorithmParam.nThresholdValOfMotionLess[2] = PARAM_MOTIONLESS_SENSITIBITY_RANG2;
    gAlgorithmParam.nThresholdValOfMotionLess[3] = PARAM_MOTIONLESS_SENSITIBITY_RANG3;
    gAlgorithmParam.nThresholdValOfMotionLess[4] = PARAM_MOTIONLESS_SENSITIBITY_RANG4;
    gAlgorithmParam.nThresholdValOfMotionLess[5] = PARAM_MOTIONLESS_SENSITIBITY_RANG5;
    gAlgorithmParam.nThresholdValOfMotionLess[6] = PARAM_MOTIONLESS_SENSITIBITY_RANG6;
    gAlgorithmParam.nThresholdValOfMotionLess[7] = PARAM_MOTIONLESS_SENSITIBITY_RANG7;
    gAlgorithmParam.nThresholdValOfMotionLess[8] = PARAM_MOTIONLESS_SENSITIBITY_RANG8;

    gAlgorithmParam.nOffTime = PARAM_OFF_TIME;

    Algo_SaveParameter((uint32_t *)&gAlgorithmParam, sizeof(ALGORITHM_PARAM_T));
    printf("[CAL] default parameters restored\r\n");
    return 0;
}

int radar_handler_set_state(const char *cmd_json)
{
    const char *cmd = ha_json_str(cmd_json, "cmd");
    if (cmd == NULL)
        return -1;

    if (strcmp(cmd, "calibrate") == 0) {
        return radar_handler_calibrate();
    }
    if (strcmp(cmd, "restore") == 0) {
        return radar_handler_restore_defaults();
    }
    if (strcmp(cmd, "push_cfg") == 0) {
        const char *ip = ha_json_str(cmd_json, "ip");
        int port = ha_json_int(cmd_json, "port", HA_PUSH_DEFAULT_PORT);
        if (ip) {
            ha_push_set_target(ip, (uint16_t)port);
        }
        return 0;
    }
    return 0;
}

void radar_handler_set_motion(uint8_t motion)
{
    g_radar_motion = motion;
    g_motion_call_count++;

    // 静止防抖：连续检测到无运动才计数
    if (motion == 2) {
        if (g_stationary_count < STATIONARY_CONFIRM_COUNT) {
            g_stationary_count++;
        }
    } else {
        g_stationary_count = 0;  // 有运动时重置计数
    }
}

uint8_t radar_handler_get_motion(void)
{
    // 连续静止次数未达到阈值时，仍判定为有运动
    if (g_radar_motion == 2 && g_stationary_count < STATIONARY_CONFIRM_COUNT) {
        return 1;  // 还没确认静止，保持 motion=1
    }
    return (g_radar_motion == 1 || g_radar_motion == 3) ? 1 : 0;
}
