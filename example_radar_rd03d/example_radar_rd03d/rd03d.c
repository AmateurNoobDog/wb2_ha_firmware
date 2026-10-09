/**
 * @file rd03d.c
 * @brief Rd-03D_V2 毫米波雷达串口驱动实现
 *
 * 上报帧解析采用逐字节状态机: 搜索帧头 -> 收满 30 字节 -> 校验帧尾,
 * 任一环节失败即丢弃已收数据重新同步, 避免串口丢字节导致后续全部错位。
 *
 * 配置命令帧格式(见 通信.md):
 *   发送: FD FC FB FA | 帧内数据长度(2B, = 命令字+命令值长度) | 命令 | 04 03 02 01
 *   应答: FD FC FB FA | 帧内数据长度(2B) | 命令字(ack) | ACK数据 | 04 03 02 01
 */
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "rd03d.h"

/* 配置帧头/帧尾 */
static const uint8_t cfg_head[RD03D_CFG_HEAD_LEN] = { 0xFD, 0xFC, 0xFB, 0xFA };
static const uint8_t cfg_tail[RD03D_CFG_TAIL_LEN] = { 0x04, 0x03, 0x02, 0x01 };
/* 上报帧头/帧尾 */
static const uint8_t rpt_head[RD03D_RPT_HEAD_LEN] = { 0xAA, 0xFF, 0x03, 0x00 };
static const uint8_t rpt_tail[2] = { 0x55, 0xCC };

/**
 * @brief 小端 16 位带符号值解码: bit15 为符号位(1 正 0 负), 低 15 位为幅值
 */
static int16_t rd03d_decode_signed(uint8_t lo, uint8_t hi)
{
    uint16_t raw = (uint16_t)lo | ((uint16_t)hi << 8);
    int16_t  mag = (int16_t)(raw & 0x7FFF);

    return (raw & 0x8000) ? mag : (int16_t)(-mag);
}

/**
 * @brief 解析单个目标的 8 字节数据: x(2) y(2) 速度(2) 像素距离(2), 均小端
 */
static void rd03d_parse_target(const uint8_t *p, rd03d_target_t *target)
{
    /* 坐标与距离全为 0 表示该目标不存在 */
    target->valid = !(p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0 &&
                      p[6] == 0 && p[7] == 0);
    target->x_mm        = rd03d_decode_signed(p[0], p[1]);
    target->y_mm        = rd03d_decode_signed(p[2], p[3]);
    target->speed_cms   = rd03d_decode_signed(p[4], p[5]);
    target->distance_mm = (uint16_t)p[6] | ((uint16_t)p[7] << 8);
}

bool rd03d_parser_input(rd03d_parser_t *parser, uint8_t byte, rd03d_frame_t *frame)
{
    if (parser == NULL || frame == NULL) {
        return false;
    }

    if (!parser->synced) {
        /* 滑动匹配帧头, 兼容帧头被截断/错位的情况 */
        if (byte == rpt_head[parser->len]) {
            parser->buf[parser->len++] = byte;
            if (parser->len == RD03D_RPT_HEAD_LEN) {
                parser->synced = true;
            }
        } else if (byte == rpt_head[0]) {
            parser->buf[0] = byte;          /* 当前字节作为新候选帧头首字节 */
            parser->len = 1;
        } else {
            parser->len = 0;
        }
        return false;
    }

    parser->buf[parser->len++] = byte;

    if (parser->len < RD03D_RPT_FRAME_LEN) {
        return false;
    }

    /* 收满一帧, 校验帧尾 */
    if (memcmp(&parser->buf[RD03D_RPT_FRAME_LEN - 2], rpt_tail, 2) == 0) {
        uint8_t i;

        frame->any_target = false;
        for (i = 0; i < RD03D_RPT_TARGET_NUM; i++) {
            rd03d_parse_target(&parser->buf[RD03D_RPT_HEAD_LEN + i * RD03D_RPT_TARGET_LEN],
                               &frame->targets[i]);
            if (frame->targets[i].valid) {
                frame->any_target = true;
            }
        }
        parser->len    = 0;
        parser->synced = false;
        return true;
    }

    /* 帧尾校验失败: 丢弃重同步(buf[0] 为 0xAA 时保留作新候选帧头首字节) */
    parser->len = (parser->buf[0] == rpt_head[0]) ? 1 : 0;
    parser->synced = false;
    return false;
}

/*----------------------------- 配置命令 -----------------------------*/

/**
 * @brief 清空串口接收缓冲(雷达在工作模式下持续上报, 旧数据会污染 ACK 判断)
 */
static void rd03d_flush_rx(hosal_uart_dev_t *uart)
{
    uint8_t tmp[128];           /* hosal_uart_receive 最多写 128 字节 */
    uint8_t i;

    for (i = 0; i < 16; i++) {
        if (hosal_uart_receive(uart, tmp, sizeof(tmp)) <= 0) {
            break;
        }
    }
}

/**
 * @brief 等待并搜索一帧 ACK 应答
 *
 * 接收流中可能混有雷达上报帧等脏数据, 因此对帧头 FD FC FB FA 做滑动匹配,
 * 组满一帧后校验 ACK 与帧尾; 校验失败则丢弃并继续搜索, 直到超时。
 *
 * @param ack_expect  期望应答(命令字 + ACK), 长度 ack_len
 * @param ack_len     ack_expect 长度
 * @param data_len    应答帧的"帧内数据长度"(= ack_len + 附加数据长度)
 * @param resp        附加数据输出缓冲(ACK 之后的内容), 可为 NULL
 * @param resp_len    附加数据长度输出, 可为 NULL
 * @return 0 成功; -1 超时; -3 ACK 不符; -4 帧尾校验失败(取最后一次错误)
 */
static int rd03d_wait_ack(hosal_uart_dev_t *uart, const uint8_t *ack_expect, uint8_t ack_len,
                          uint8_t data_len, uint8_t *resp, uint8_t *resp_len)
{
    uint8_t  frame[RD03D_CFG_MAX_FRAME];
    uint8_t  tmp[128];
    uint8_t  idx = 0;                       /* 已收集字节数(从帧头起) */
    uint8_t  need = (uint8_t)(RD03D_CFG_HEAD_LEN + 2 + data_len + RD03D_CFG_TAIL_LEN);
    int      timeout = 200;                  /* 200 * 10ms = 2s */
    int      last_err = -1;
    int      ret;
    int      i;

    while (timeout-- > 0) {
        ret = hosal_uart_receive(uart, tmp, sizeof(tmp));
        if (ret <= 0) {
            vTaskDelay(10);
            continue;
        }

        for (i = 0; i < ret; i++) {
            uint8_t byte = tmp[i];

            if (idx < RD03D_CFG_HEAD_LEN) {
                /* 滑动匹配帧头, 兼容帧头前有脏数据/被截断 */
                if (byte == cfg_head[idx]) {
                    frame[idx++] = byte;
                } else if (byte == cfg_head[0]) {
                    frame[0] = byte;        /* 当前字节作为新候选帧头首字节 */
                    idx = 1;
                } else {
                    idx = 0;
                }
                continue;
            }

            frame[idx++] = byte;
            if (idx < need) {
                continue;
            }

            /* 收满一帧, 校验帧尾与 ACK */
            if (memcmp(&frame[need - RD03D_CFG_TAIL_LEN], cfg_tail, RD03D_CFG_TAIL_LEN) != 0) {
                last_err = -4;
                idx = 0;
                continue;
            }
            if (memcmp(&frame[RD03D_CFG_HEAD_LEN + 2], ack_expect, ack_len) != 0) {
                last_err = -3;
                idx = 0;
                continue;
            }

            if (resp != NULL && resp_len != NULL) {
                *resp_len = (uint8_t)(data_len - ack_len);
                if (*resp_len > 0) {
                    memcpy(resp, &frame[RD03D_CFG_HEAD_LEN + 2 + ack_len], *resp_len);
                }
            }
            return 0;
        }
    }

    return last_err;
}

/**
 * @brief 发送一帧配置命令并接收 ACK
 *
 * @param uart        雷达串口
 * @param cmd         命令字 + 命令值
 * @param cmd_len     命令长度(2 或 4), 即发送帧的"帧内数据长度"
 * @param ack_expect  期望应答(命令字 + ACK), 长度 ack_len
 * @param ack_len     ack_expect 长度
 * @param data_len    应答帧的"帧内数据长度"(= ack_len + 附加数据长度)
 * @param resp        附加数据输出缓冲(ACK 之后的内容), 可为 NULL
 * @param resp_len    附加数据长度输出, 可为 NULL
 * @return 0 成功
 */
static int rd03d_send_cmd(hosal_uart_dev_t *uart, const uint8_t *cmd, uint8_t cmd_len,
                          const uint8_t *ack_expect, uint8_t ack_len, uint8_t data_len,
                          uint8_t *resp, uint8_t *resp_len)
{
    uint8_t tx[RD03D_CFG_HEAD_LEN + 2 + 4 + RD03D_CFG_TAIL_LEN];
    uint8_t idx = 0;
    int     ret;

    /* 组帧: 帧头 + 帧内数据长度 + 命令 + 帧尾 */
    memcpy(&tx[idx], cfg_head, RD03D_CFG_HEAD_LEN);
    idx += RD03D_CFG_HEAD_LEN;
    tx[idx++] = cmd_len;        /* 帧内数据长度(小端低字节) */
    tx[idx++] = 0x00;
    memcpy(&tx[idx], cmd, cmd_len);
    idx += cmd_len;
    memcpy(&tx[idx], cfg_tail, RD03D_CFG_TAIL_LEN);
    idx += RD03D_CFG_TAIL_LEN;

    /* 发送前必须清空 RX: 雷达在工作模式持续上报, FIFO 里的旧帧会干扰 ACK 判断 */
    rd03d_flush_rx(uart);

    hosal_uart_send(uart, tx, idx);

    ret = rd03d_wait_ack(uart, ack_expect, ack_len, data_len, resp, resp_len);

    /* 清空残留数据(如雷达未及时切换模式又上报的帧) */
    rd03d_flush_rx(uart);

    return ret;
}

int rd03d_enable_config(hosal_uart_dev_t *uart)
{
    /* 发送: FF00 0100; 应答: FF01 0000 0100 4000 (协议版本 + 缓冲区大小) */
    static const uint8_t cmd[4] = { 0xFF, 0x00, 0x01, 0x00 };
    static const uint8_t ack[4] = { 0xFF, 0x01, 0x00, 0x00 };

    return rd03d_send_cmd(uart, cmd, sizeof(cmd), ack, sizeof(ack), 8, NULL, NULL);
}

int rd03d_end_config(hosal_uart_dev_t *uart)
{
    /* 发送: FE00; 应答: FE01 0000 */
    static const uint8_t cmd[2] = { 0xFE, 0x00 };
    static const uint8_t ack[4] = { 0xFE, 0x01, 0x00, 0x00 };

    return rd03d_send_cmd(uart, cmd, sizeof(cmd), ack, sizeof(ack), 4, NULL, NULL);
}

int rd03d_set_mode(hosal_uart_dev_t *uart, rd03d_mode_t mode)
{
    static const uint8_t cmd_single[2] = { 0x80, 0x00 };
    static const uint8_t cmd_multi[2]  = { 0x90, 0x00 };
    static const uint8_t ack_single[4] = { 0x80, 0x01, 0x00, 0x00 };
    static const uint8_t ack_multi[4]  = { 0x90, 0x01, 0x00, 0x00 };

    if (mode == RD03D_MODE_SINGLE) {
        return rd03d_send_cmd(uart, cmd_single, 2, ack_single, 4, 4, NULL, NULL);
    }
    return rd03d_send_cmd(uart, cmd_multi, 2, ack_multi, 4, 4, NULL, NULL);
}

int rd03d_query_version(hosal_uart_dev_t *uart, char *version, uint32_t len)
{
    static const uint8_t cmd[2] = { 0x00, 0x00 };
    static const uint8_t ack[4] = { 0x00, 0x01, 0x00, 0x00 };
    uint8_t  data[32] = { 0 };
    uint8_t  data_len = 0;
    int      ret;

    /* 应答帧内数据: 命令字(0001) + ACK(0000) + 版本号长度(2B) + 版本号 */
    ret = rd03d_send_cmd(uart, cmd, sizeof(cmd), ack, sizeof(ack), 14, data, &data_len);
    if (ret != 0) {
        return ret;
    }

    if (version != NULL && len > 0 && data_len >= 2) {
        uint8_t vlen = data[1] ? data[1] : data[0];

        if (vlen > data_len - 2) {
            vlen = data_len - 2;
        }
        if (vlen > len - 1) {
            vlen = (uint8_t)(len - 1);
        }
        memcpy(version, &data[2], vlen);
        version[vlen] = '\0';
    }

    return 0;
}
