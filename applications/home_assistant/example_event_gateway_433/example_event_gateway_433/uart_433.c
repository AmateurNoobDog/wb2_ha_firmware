#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <hosal_uart.h>
#include "uart_433.h"
#include "app_config.h"

static hosal_uart_dev_t uart_0 = {
    .config = {
        .uart_id = 0,
        .tx_pin = 16,
        .rx_pin = 7,
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = 115200,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity = HOSAL_NO_PARITY,
        .stop_bits = HOSAL_STOP_BITS_1,
        .mode = HOSAL_UART_MODE_POLL,
    },
};

static hosal_uart_dev_t uart_433 = {
    .config = {
        .uart_id = UART_433_ID,
        .tx_pin = UART_433_TX_PIN,
        .rx_pin = UART_433_RX_PIN,
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = UART_433_BAUD,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity = HOSAL_NO_PARITY,
        .stop_bits = HOSAL_STOP_BITS_1,
        .mode = HOSAL_UART_MODE_POLL,
    },
};

static void uart0_send(const uint8_t *data, uint16_t len)
{
    hosal_uart_send(&uart_0, data, len);
}

void uart_433_log(const char *msg)
{
    uart0_send((const uint8_t *)msg, strlen(msg));
}

void uart_433_init(void)
{
    hosal_uart_init(&uart_0);
    uart_433_log("[433] uart0 init done\r\n");

    hosal_uart_init(&uart_433);
    uart_433_log("[433] uart1 init done POLL\r\n");
}

int uart_433_read(uint8_t *buf, int buf_len)
{
    return hosal_uart_receive(&uart_433, buf, buf_len);
}

void uart_433_send(const uint8_t *data, int len)
{
    hosal_uart_send(&uart_433, data, len);
}
