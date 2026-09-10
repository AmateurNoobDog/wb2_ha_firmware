#ifndef __HA_PUSH_H__
#define __HA_PUSH_H__

#include <stdint.h>

/* Initialize push module from NVS config.
 * Call this after WiFi got IP. */
void ha_push_init(void);

/* Set HA target IP and port, save to NVS. */
void ha_push_set_target(const char *ip, uint16_t port);

/* Clear push config and disable push. */
void ha_push_clear(void);

/* Send JSON state to HA via TCP (non-blocking, fails silently). */
void ha_push_send(const char *json);

/* Returns 1 if push target is configured, 0 otherwise. */
uint8_t ha_push_enabled(void);

#endif /* __HA_PUSH_H__ */
