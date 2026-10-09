#ifndef TW_TTS_APP_H
#define TW_TTS_APP_H

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include "uart_app.h"

void TTS_set_speed(uint8_t speed);
void TTS_set_volume(uint8_t volume);
void TTS(const char *chinese);

#endif
