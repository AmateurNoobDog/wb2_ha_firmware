#include <stdio.h>
#include <string.h>

#include <easyflash.h>
#include <cli.h>

#include "store.h"
#include "app_config.h"

extern int easyflash_cli_init(void);

void store_init(void)
{
    easyflash_cli_init();
}

static bool store_get_str(const char *key, char *buf, int len)
{
    size_t read_len = 0;
    size_t ret;

    if (len <= 0) {
        return false;
    }
    ret = ef_get_env_blob(key, (uint8_t *)buf, len - 1, &read_len);
    if (ret == 0) {
        buf[0] = '\0';
        return false;
    }
    buf[read_len] = '\0';
    return true;
}

static bool store_set_bytes(const char *key, const uint8_t *value, int len)
{
    bool ok = ef_set_env_blob(key, value, len) == EF_NO_ERR;

    if (ok) {
        ef_save_env();
    }
    return ok;
}

bool store_wifi_load(store_wifi_t *cfg)
{
    bool has;

    memset(cfg, 0, sizeof(*cfg));
    has = store_get_str(STORE_KEY_SSID, cfg->ssid, sizeof(cfg->ssid));
    store_get_str(STORE_KEY_PWD, cfg->pwd, sizeof(cfg->pwd));
    return has;
}

bool store_has_wifi(void)
{
    store_wifi_t cfg;

    return store_wifi_load(&cfg);
}

void store_wifi_save_ssid(const uint8_t *ssid, int len)
{
    bool ok = store_set_bytes(STORE_KEY_SSID, ssid, len);

    printf("[STORE] save ssid (%d bytes) -> %s\n", len, ok ? "ok" : "FAIL");
}

void store_wifi_save_pwd(const uint8_t *pwd, int len)
{
    bool ok = store_set_bytes(STORE_KEY_PWD, pwd, len);

    printf("[STORE] save pwd (%d bytes) -> %s\n", len, ok ? "ok" : "FAIL");
}

void store_wifi_clear(void)
{
    ef_del_env(STORE_KEY_SSID);
    ef_del_env(STORE_KEY_PWD);
    printf("[STORE] wifi config cleared\n");
}

bool store_push_load(store_push_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    bool has_ip = store_get_str(STORE_KEY_HA_IP, cfg->ip, sizeof(cfg->ip));
    uint8_t port_buf[2] = {0};
    size_t read_len = 0;
    ef_get_env_blob(STORE_KEY_HA_PORT, port_buf, sizeof(port_buf), &read_len);
    if (read_len == 2) {
        cfg->port = (uint16_t)(port_buf[0] | (port_buf[1] << 8));
    } else {
        cfg->port = HA_PUSH_DEFAULT_PORT;
    }
    cfg->enabled = has_ip ? 1 : 0;
    return cfg->enabled;
}

void store_push_save(const char *ip, uint16_t port)
{
    bool ok1 = store_set_bytes(STORE_KEY_HA_IP, (const uint8_t *)ip, strlen(ip));
    uint8_t port_buf[2] = { (uint8_t)(port & 0xFF), (uint8_t)(port >> 8) };
    bool ok2 = store_set_bytes(STORE_KEY_HA_PORT, port_buf, 2);
    printf("[STORE] save push config: ip=%s port=%d -> %s\n",
           ip, port, (ok1 && ok2) ? "ok" : "FAIL");
}

void store_push_clear(void)
{
    ef_del_env(STORE_KEY_HA_IP);
    ef_del_env(STORE_KEY_HA_PORT);
    printf("[STORE] push config cleared\n");
}

static void cmd_cfg_clear(char *buf, int len, int argc, char **argv)
{
    store_wifi_clear();
}

const static struct cli_command store_cmds[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"cfg_clear", "clear wifi config", cmd_cfg_clear},
};