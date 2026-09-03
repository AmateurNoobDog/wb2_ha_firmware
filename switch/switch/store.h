#ifndef __STORE_H__
#define __STORE_H__

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

typedef struct {
    char ssid[STORE_SSID_MAX];
    char pwd[STORE_PWD_MAX];
} store_wifi_t;

void store_init(void);
bool store_wifi_load(store_wifi_t *cfg);
bool store_has_wifi(void);
void store_wifi_save_ssid(const uint8_t *ssid, int len);
void store_wifi_save_pwd(const uint8_t *pwd, int len);
void store_wifi_clear(void);

#endif /* __STORE_H__ */