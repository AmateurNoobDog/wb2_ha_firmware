/**
 * @file rd03d.h
 * @brief Rd-03D_V2 毫米波雷达串口驱动（协议见同目录 通信.md）
 *
 * 串口参数: 256000 (默认), 1 停止位, 无校验
 * 上报帧:   AA FF 03 00 | 目标1(8B) 目标2(8B) 目标3(8B) | 55 CC   共 30 字节
 * 配置帧头: FD FC FB FA, 配置帧尾: 04 03 02 01
 */
#ifndef __RD03D_H__
#define __RD03D_H__

#include <stdint.h>
#include <stdbool.h>
#include <hosal_uart.h>
#include "app_config.h"

/*-------------------------- 硬件配置 --------------------------*/
/* 串口引脚/波特率(RD03D_UART_*)统一在 app_config.h 中配置, 见该文件注释:
 * 雷达使用 UART1 控制器, 引脚 GPIO4/GPIO5, 256000 8N1。
 * 日志口(系统 console)为 UART0(GPIO16/GPIO7), 本工程不可占用。 */

/*----------------------------- 协议常量 -----------------------------*/
#define RD03D_RPT_HEAD_LEN      4       /* 上报帧头长度 AA FF 03 00 */
#define RD03D_RPT_TARGET_NUM    3       /* 最多 3 个目标 */
#define RD03D_RPT_TARGET_LEN    8       /* 单目标数据长度 */
#define RD03D_RPT_FRAME_LEN     (RD03D_RPT_HEAD_LEN + RD03D_RPT_TARGET_NUM * RD03D_RPT_TARGET_LEN + 2) /* 30 */
#define RD03D_CFG_HEAD_LEN      4       /* 配置帧头 FD FC FB FA */
#define RD03D_CFG_TAIL_LEN      4       /* 配置帧尾 04 03 02 01 */
#define RD03D_CFG_MAX_FRAME     64      /* 配置应答最大长度 */

/* 单/多目标模式 */
typedef enum {
    RD03D_MODE_SINGLE = 0,
    RD03D_MODE_MULTI  = 1,
} rd03d_mode_t;

/* 单个目标信息 */
typedef struct {
    bool     valid;         /* 该目标是否存在 */
    int16_t  x_mm;          /* X 坐标, 单位 mm, 正方向见模组文档 */
    int16_t  y_mm;          /* Y 坐标, 单位 mm */
    int16_t  speed_cms;     /* 速度, 单位 cm/s, 靠近雷达为负 */
    uint16_t distance_mm;   /* 像素距离值, 单位 mm */
} rd03d_target_t;

/* 一帧解析结果 */
typedef struct {
    rd03d_target_t targets[RD03D_RPT_TARGET_NUM];
    bool any_target;        /* 是否存在任一目标 */
} rd03d_frame_t;

/* 解析器状态(内部使用, 用户只需定义实例) */
typedef struct {
    uint8_t  buf[RD03D_RPT_FRAME_LEN];
    uint16_t len;
    bool     synced;        /* 是否已匹配到帧头 */
} rd03d_parser_t;

/**
 * @brief 喂入一个字节进行帧解析
 * @param parser 解析器实例
 * @param byte   输入字节
 * @param frame  解析成功时输出本帧数据
 * @return true 解析出一帧有效数据
 */
bool rd03d_parser_input(rd03d_parser_t *parser, uint8_t byte, rd03d_frame_t *frame);

/*------------------------- 配置命令(需先使能配置) -------------------------*/

/**
 * @brief 使能配置命令, 其他配置命令必须在此命令之后下发才有效
 * @param uart 雷达串口设备
 * @return 0 成功, 其他失败
 */
int rd03d_enable_config(hosal_uart_dev_t *uart);

/**
 * @brief 结束配置命令, 执行后雷达恢复工作模式
 */
int rd03d_end_config(hosal_uart_dev_t *uart);

/**
 * @brief 单/多目标模式切换
 * @param mode RD03D_MODE_SINGLE 单目标 / RD03D_MODE_MULTI 多目标
 */
int rd03d_set_mode(hosal_uart_dev_t *uart, rd03d_mode_t mode);

/**
 * @brief 查询版本号
 * @param version 输出版本号字符串缓冲区
 * @param len     缓冲区长度
 * @return 0 成功
 */
int rd03d_query_version(hosal_uart_dev_t *uart, char *version, uint32_t len);

#endif /*__RD03D_H__*/
