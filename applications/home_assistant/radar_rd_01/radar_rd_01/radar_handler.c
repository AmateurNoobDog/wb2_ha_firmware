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

static uint8_t g_radar_motion = 0;
static uint32_t g_state_call_count = 0;
static uint32_t g_motion_call_count = 0;

void radar_handler_init(void)
{
}

int radar_handler_get_state(char *buf, int buf_len)
{
    g_state_call_count++;
#if RADAR_GATE_DATA_ENABLE
    return snprintf(buf, buf_len,
                    "\"model\":\"%s\",\"motion\":%d,\"on\":%d,"
                    "\"g0\":%d,\"g1\":%d,\"g2\":%d,\"g3\":%d,"
                    "\"g4\":%d,\"g5\":%d,\"g6\":%d,\"g7\":%d,"
                    "\"scnt\":%lu,\"mcnt\":%lu",
                    DEVICE_MODEL, g_radar_motion, g_radar_motion,
                    NormalDopplerMaxVal[1], NormalDopplerMaxVal[2],
                    NormalDopplerMaxVal[3], NormalDopplerMaxVal[4],
                    NormalDopplerMaxVal[5], NormalDopplerMaxVal[6],
                    NormalDopplerMaxVal[7], NormalDopplerMaxVal[8],
                    (unsigned long)g_state_call_count,
                    (unsigned long)g_motion_call_count);
#elif RADAR_DEBUG_COUNTER_ENABLE
    return snprintf(buf, buf_len,
                    "\"model\":\"%s\",\"motion\":%d,\"on\":%d,"
                    "\"scnt\":%lu,\"mcnt\":%lu",
                    DEVICE_MODEL, g_radar_motion, g_radar_motion,
                    (unsigned long)g_state_call_count,
                    (unsigned long)g_motion_call_count);
#else
    return snprintf(buf, buf_len,
                    "\"model\":\"%s\",\"sw_version\":\"%s\",\"motion\":%d,\"presence\":%d,\"push\":%d",
                    DEVICE_MODEL, DEVICE_SW_VERSION,
                    (g_radar_motion == 1 || g_radar_motion == 3) ? 1 : 0,
                    (g_radar_motion >= 1 && g_radar_motion <= 3) ? 1 : 0,
                    ha_push_enabled());
#endif
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

    if (strncmp(cmd, "calibrate", 9) == 0) {
        return radar_handler_calibrate();
    }
    if (strncmp(cmd, "restore", 7) == 0) {
        return radar_handler_restore_defaults();
    }
    if (strncmp(cmd, "push_cfg", 8) == 0) {
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
}

uint8_t radar_handler_get_motion(void)
{
    return g_radar_motion;
}
