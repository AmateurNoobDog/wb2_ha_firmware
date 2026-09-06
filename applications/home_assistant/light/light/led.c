#include "led.h"
#include <bl_pwm.h>

#include "app_config.h"

static uint8_t s_r = 0;
static uint8_t s_g = 0;
static uint8_t s_b = 0;
static uint8_t s_brightness = 255;

static void led_set_channel(uint8_t ch, uint8_t level)
{
    bl_pwm_set_duty(ch, level * 100.0f / 255.0f);
}

void led_init(void)
{
    bl_pwm_init(LED_RED_CH, LED_RED_PIN, LED_PWM_FREQ);
    bl_pwm_init(LED_GREEN_CH, LED_GREEN_PIN, LED_PWM_FREQ);
    bl_pwm_init(LED_BLUE_CH, LED_BLUE_PIN, LED_PWM_FREQ);
    bl_pwm_start(LED_RED_CH);
    bl_pwm_start(LED_GREEN_CH);
    bl_pwm_start(LED_BLUE_CH);

    s_r = 0;
    s_g = 0;
    s_b = 0;
    s_brightness = 255;
    led_set_channel(LED_RED_CH, 0);
    led_set_channel(LED_GREEN_CH, 0);
    led_set_channel(LED_BLUE_CH, 0);
}

void led_set_state(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    s_r = r;
    s_g = g;
    s_b = b;
    s_brightness = brightness;
    led_set_channel(LED_RED_CH,   s_r   * s_brightness / 255);
    led_set_channel(LED_GREEN_CH, s_g   * s_brightness / 255);
    led_set_channel(LED_BLUE_CH,  s_b   * s_brightness / 255);
}

void led_get_state(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *brightness)
{
    if (r != NULL) {
        *r = s_r;
    }
    if (g != NULL) {
        *g = s_g;
    }
    if (b != NULL) {
        *b = s_b;
    }
    if (brightness != NULL) {
        *brightness = s_brightness;
    }
}
