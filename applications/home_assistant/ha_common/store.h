#ifndef __STORE_H__
#define __STORE_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char ssid[64];
    char pwd[64];
} store_wifi_t;

typedef struct {
    char ip[16];
    uint16_t port;
    uint8_t enabled;     // 1 if HA IP is configured
} store_push_t;

void store_init(void);

bool store_wifi_load(store_wifi_t *cfg);
bool store_has_wifi(void);
void store_wifi_save_ssid(const uint8_t *ssid, int len);
void store_wifi_save_pwd(const uint8_t *pwd, int len);
void store_wifi_clear(void);

bool store_push_load(store_push_t *cfg);
void store_push_save(const char *ip, uint16_t port);
void store_push_clear(void);

#endif /* __STORE_H__ */
