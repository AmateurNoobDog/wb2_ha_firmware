#ifndef __RADAR_HANDLER_H__
#define __RADAR_HANDLER_H__

#include <stdint.h>

void radar_handler_init(void);
int radar_handler_get_state(char *buf, int buf_len);
int radar_handler_set_state(const char *cmd_json);
void radar_handler_set_motion(uint8_t motion);
int radar_handler_calibrate(void);
int radar_handler_restore_defaults(void);

#endif /* __RADAR_HANDLER_H__ */
