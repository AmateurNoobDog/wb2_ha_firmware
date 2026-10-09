/**
 * @file uart_app.h
 * @brief UART 薄封装(移植自 example_notify_tts, 已修复 sizeof(data) 接收 bug)
 *
 * uart0: 调试日志口 GPIO16/7 @115200(与系统 console 同参数)
 * uart1: 雷达口 GPIO4/5 @256000, 由 rd03d.c 通过 extern uart_1 直接使用
 */
#ifndef UART_APP_H
#define UART_APP_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <blog.h>
#include <hosal_uart.h>

/* uart1 对 rd03d.c 可见(其 API 收 hosal_uart_dev_t*) */
extern hosal_uart_dev_t uart_1;

void uart_init(void);

void uart0_send(const uint8_t *data, uint32_t length);
void uart1_send(const uint8_t *data, uint32_t length);

/* 接收: buf_len 必须是调用方缓冲区的真实长度 */
uint16_t uart0_get(uint8_t *data, uint16_t buf_len);
uint16_t uart1_get(uint8_t *data, uint16_t buf_len);

void uart0_print(const char *data);
void uart1_print(const char *data);

/* 走 uart0 直写日志(不带换行时自动补 \r\n) */
void uart_log(const char *msg);
/* 格式化日志(自动补 \r\n), 用于替代 blog_info 的格式化输出 */
void uart_logf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
