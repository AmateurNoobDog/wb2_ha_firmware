/**
 * @file uart_app.c
 * @brief UART 薄封装(移植自 example_notify_tts)
 *
 * 相对原版的修复:
 *   1. uart*_get 原为 sizeof(data)(指针 4 字节) —— 改为显式 buf_len
 *   2. uart*_print 手写求长循环 —— 改用 strlen
 *   3. uart_0 收为 static, uart_1 保持全局供 rd03d.c 使用
 *   4. 新增 uart_logf(格式化) 与换行自动补齐, 以替代 blog_* 日志
 */
#include <stdarg.h>
#include "uart_app.h"
#include "app_config.h"

/* 日志口: 与系统 console 同引脚同波特率, 重复 init 无害 */
static hosal_uart_dev_t uart_0 = {
    .config = {
        .uart_id    = 0,
        .tx_pin     = UART_DEBUG_TX_PIN,
        .rx_pin     = UART_DEBUG_RX_PIN,
        .cts_pin    = 255,
        .rts_pin    = 255,
        .baud_rate  = UART_DEBUG_BAUDRATE,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity     = HOSAL_NO_PARITY,
        .stop_bits  = HOSAL_STOP_BITS_1,
        .mode       = HOSAL_UART_MODE_POLL,
    },
};

/* 雷达口: 参数取自 RD03D_* 宏 */
hosal_uart_dev_t uart_1 = {
    .config = {
        .uart_id    = RD03D_UART_ID,
        .tx_pin     = RD03D_UART_TX_PIN,
        .rx_pin     = RD03D_UART_RX_PIN,
        .cts_pin    = 255,
        .rts_pin    = 255,
        .baud_rate  = RD03D_UART_BAUDRATE,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity     = HOSAL_NO_PARITY,
        .stop_bits  = HOSAL_STOP_BITS_1,
        .mode       = HOSAL_UART_MODE_POLL,
    },
};

void uart_init(void)
{
    hosal_uart_init(&uart_0);
    hosal_uart_init(&uart_1);
}

void uart0_send(const uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0) {
        return;
    }
    hosal_uart_send(&uart_0, data, length);
}

void uart1_send(const uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0) {
        return;
    }
    hosal_uart_send(&uart_1, data, length);
}

uint16_t uart0_get(uint8_t *data, uint16_t buf_len)
{
    int length;

    if (data == NULL || buf_len == 0) {
        return 0;
    }
    length = hosal_uart_receive(&uart_0, data, buf_len);
    return (length > 0) ? (uint16_t)length : 0;
}

uint16_t uart1_get(uint8_t *data, uint16_t buf_len)
{
    int length;

    if (data == NULL || buf_len == 0) {
        return 0;
    }
    length = hosal_uart_receive(&uart_1, data, buf_len);
    return (length > 0) ? (uint16_t)length : 0;
}

void uart0_print(const char *data)
{
    if (data == NULL) {
        return;
    }
    hosal_uart_send(&uart_0, data, strlen(data));
}

void uart1_print(const char *data)
{
    if (data == NULL) {
        return;
    }
    hosal_uart_send(&uart_1, data, strlen(data));
}

/* 末尾不是 '\n' 时补 \r\n, 保证每条日志独占一行 */
static void uart_log_flush(const char *buf, int len)
{
    hosal_uart_send(&uart_0, buf, (uint32_t)len);
    if (len <= 0 || buf[len - 1] != '\n') {
        hosal_uart_send(&uart_0, "\r\n", 2);
    }
}

void uart_log(const char *msg)
{
    if (msg == NULL) {
        return;
    }
    uart_log_flush(msg, (int)strlen(msg));
}

void uart_logf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    int n;

    if (fmt == NULL) {
        return;
    }
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }
    if (n >= (int)sizeof(buf)) {
        n = (int)sizeof(buf) - 1;   /* 已截断 */
    }
    uart_log_flush(buf, n);
}
