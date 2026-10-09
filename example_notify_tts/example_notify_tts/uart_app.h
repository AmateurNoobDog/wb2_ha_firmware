#ifndef UART_APP_H
#define UART_APP_H

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <blog.h>
#include <hosal_uart.h>

void uart_init();

void uart0_send(uint8_t *data, uint16_t length);
void uart1_send(uint8_t *data, uint16_t length);

uint16_t uart0_get(uint8_t *data);
uint16_t uart1_get(uint8_t *data);

void uart0_print(char* data);
void uart1_print(char* data);
void uart_log(const char *msg);

#endif
