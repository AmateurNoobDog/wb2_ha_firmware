#ifndef __LED_H__
#define __LED_H__

#include <stdint.h>

void led_init(void);
void led_set_state(uint8_t r, uint8_t g, uint8_t b);
void led_get_state(uint8_t *r, uint8_t *g, uint8_t *b);

#endif /* __LED_H__ */