#ifndef __SWITCH_HANDLER_H__
#define __SWITCH_HANDLER_H__

int switch_handler_get_state(char *buf, int buf_len);
int switch_handler_set_state(const char *cmd_json);

#endif /* __SWITCH_HANDLER_H__ */