#include <bl_gpio.h>

#include "relay.h"
#include "app_config.h"

static const int s_pins[SWITCH_COUNT] = SWITCH_PINS;
static bool s_on[SWITCH_COUNT];

void relay_init(void)
{
    int i;

    for (i = 0; i < SWITCH_COUNT; i++) {
        bl_gpio_enable_output(s_pins[i], 1, 0);
    }
    for (i = 0; i < SWITCH_COUNT; i++) {
        relay_set(i, false);
    }
}

void relay_set(int idx, bool on)
{
    if (idx < 0 || idx >= SWITCH_COUNT) {
        return;
    }
    s_on[idx] = on;

#if RELAY_ACTIVE_HIGH
    bl_gpio_output_set(s_pins[idx], on ? 1 : 0);
#else
    bl_gpio_output_set(s_pins[idx], on ? 0 : 1);
#endif
}

bool relay_get(int idx)
{
    if (idx < 0 || idx >= SWITCH_COUNT) {
        return false;
    }
    return s_on[idx];
}