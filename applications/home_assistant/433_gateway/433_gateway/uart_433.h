#ifndef __UART_433_H__
#define __UART_433_H__

#include <stdint.h>

void uart_433_log(const char *msg);
void uart_433_init(void);
int uart_433_read(uint8_t *buf, int buf_len);
void uart_433_send(const uint8_t *data, int len);

#endif /* __UART_433_H__ */
