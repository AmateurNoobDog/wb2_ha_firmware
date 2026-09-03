#ifndef __HA_JSON_H__
#define __HA_JSON_H__

/* Minimal JSON field accessors for the fixed device protocol. */

const char *ha_json_str(const char *buf, const char *key);
int ha_json_int(const char *buf, const char *key, int def);

#endif /* __HA_JSON_H__ */