/**
 * @file rd03d_handler.h
 * @brief Home Assistant 设备回调与雷达数据缓存
 *
 * 实体布局(entity seq, id = {MAC 12位hex}_{seq}):
 *   seq 1      : binary_sensor 有人(any_target)
 *   seq 2-4    : sensor 目标1 X / Y / 速度
 *   seq 5-7    : sensor 目标2 X / Y / 速度
 *   seq 8-10   : sensor 目标3 X / Y / 速度
 */
#ifndef __RD03D_HANDLER_H__
#define __RD03D_HANDLER_H__

#include <stdint.h>
#include "rd03d.h"

/* 实体序号 */
#define RD03D_ENTITY_PRESENCE    1
#define RD03D_ENTITY_TARGET_BASE 2      /* 目标i: BASE + i*3 + {0:X, 1:Y, 2:速度} */
#define RD03D_ENTITY_TOTAL       10

/* ha_device_t 回调 */
int rd03d_handler_get_device(char *buf, int buf_len);
int rd03d_handler_get_state(char *buf, int buf_len);
int rd03d_handler_set_state(const char *cmd_json);

/* 生成实体 ID: {MAC hex}_{seq} */
void rd03d_gen_entity_id(char *buf, int buf_len, const uint8_t *mac, int seq);

/* 雷达任务写入最新一帧(内部做短临界区拷贝, 保证 get_state 读到完整帧) */
void rd03d_handler_update(const rd03d_frame_t *frame);

/* TCP 任务读取最新一帧, valid=false 表示尚未收到过有效帧 */
void rd03d_handler_get_frame(rd03d_frame_t *frame, int *valid);

#endif /* __RD03D_HANDLER_H__ */
