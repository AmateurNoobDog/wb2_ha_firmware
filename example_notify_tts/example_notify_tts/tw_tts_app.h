#ifndef TW_TTS_APP_H
#define TW_TTS_APP_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>
#include "uart_app.h"

/* 帧头 */
#define TTS_FRAME_HEAD          0xFD

/* 单次合成文本上限：手册规定不超过 4K 字节（扣除编码参数 2 字节） */
#define TTS_MAX_TEXT_BYTES      4094

/* 命令字 */
#define TTS_CMD_START           0x01    /* 开始合成 */
#define TTS_CMD_STOP            0x02    /* 停止合成 */
#define TTS_CMD_PAUSE           0x03    /* 暂停合成 */
#define TTS_CMD_RESUME          0x04    /* 继续合成 */
#define TTS_CMD_QUERY           0x21    /* 查询当前状态 */

/* 编码参数（只对合成播报的文本有效） */
#define TTS_ENC_GB2312          0x00    /* GB2312 */
#define TTS_ENC_UTF8            0x04    /* UTF-8 */
#define TTS_ENC_CTRL            0x01    /* 控制标记帧（[v*]/[s*]/[t*]/提示音名称） */

/* 回传指令（暂不解析，预留） */
#define TTS_ACK_OK              0x41    /* 收到正确的命令帧 */
#define TTS_ACK_CMD_ERR         0x45    /* 命令字错误 */
#define TTS_ACK_LEN_ERR         0x46    /* 数据不完整或帧长度错误 */
#define TTS_ACK_INIT_DONE       0x4A    /* 系统初始化完成 */
#define TTS_ACK_PLAYING         0x4E    /* 播放状态 */
#define TTS_ACK_IDLE            0x4F    /* 空闲状态 */

/* 参数取值：手册规定 10 级，默认中间值 5 */
#define TTS_PARAM_MIN           0
#define TTS_PARAM_MAX           9

void TTS(const char *chinese);
void TTS_set_volume(uint8_t volume);
void TTS_set_speed(uint8_t speed);
void TTS_set_tone(uint8_t tone);
void TTS_stop(void);
void TTS_pause(void);
void TTS_resume(void);
void TTS_query_status(void);
void TTS_play_ring(uint8_t index);
void TTS_play_message(uint8_t index);
void TTS_play_alert(uint8_t index);

#endif
