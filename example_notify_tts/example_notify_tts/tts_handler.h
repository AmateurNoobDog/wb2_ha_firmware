#ifndef TTS_HANDLER_H
#define TTS_HANDLER_H

void tts_handler_init(void);
int tts_handler_get_device(char *buf, int buf_len);
int tts_handler_get_state(char *buf, int buf_len);
int tts_handler_set_state(const char *cmd_json);

#endif
