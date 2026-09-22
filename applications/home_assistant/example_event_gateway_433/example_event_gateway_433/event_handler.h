#ifndef __EVENT_HANDLER_H__
#define __EVENT_HANDLER_H__

void event_handler_init(void);
int event_handler_get_device(char *buf, int buf_len);
int event_handler_get_state(char *buf, int buf_len);
int event_handler_set_state(const char *cmd_json);

#endif /* __EVENT_HANDLER_H__ */
