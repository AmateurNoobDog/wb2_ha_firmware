#ifndef __LED_HANDLER_H__
#define __LED_HANDLER_H__

int led_handler_get_device(char *buf, int buf_len);
int led_handler_get_state(char *buf, int buf_len);
int led_handler_set_state(const char *cmd_json);

#endif /* __LED_HANDLER_H__ */
