#include <stdio.h>
#include <string.h>
#include <lwip/api.h>
#include <lwip/ip_addr.h>
#include "ha_push.h"
#include "store.h"
#include "app_config.h"

static ip_addr_t s_ha_ip;
static uint16_t s_ha_port = HA_PUSH_DEFAULT_PORT;
static uint8_t s_push_enabled = 0;

void ha_push_init(void)
{
    store_push_t cfg;
    if (store_push_load(&cfg) && cfg.enabled && cfg.ip[0] != '\0') {
        ha_push_set_target(cfg.ip, cfg.port);
        printf("[PUSH] loaded HA target: %s:%d\n", cfg.ip, cfg.port);
    } else {
        printf("[PUSH] no HA target configured, push disabled\n");
    }
}

void ha_push_set_target(const char *ip, uint16_t port)
{
    if (!ip || !ipaddr_aton(ip, &s_ha_ip)) {
        return;
    }
    s_ha_port = (port > 0) ? port : HA_PUSH_DEFAULT_PORT;
    s_push_enabled = 1;
    store_push_save(ip, s_ha_port);
}

uint8_t ha_push_enabled(void)
{
    return s_push_enabled;
}

void ha_push_clear(void)
{
    s_push_enabled = 0;
    memset(&s_ha_ip, 0, sizeof(s_ha_ip));
    s_ha_port = HA_PUSH_DEFAULT_PORT;
    store_push_clear();
    printf("[PUSH] push disabled and config cleared\n");
}

void ha_push_send(const char *json)
{
    struct netconn *conn;
    err_t err;

    if (!s_push_enabled || !json || json[0] == '\0') {
        return;
    }

    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL) {
        return;
    }

    netconn_set_recvtimeout(conn, 2000);

    err = netconn_connect(conn, &s_ha_ip, s_ha_port);
    if (err != ERR_OK) {
        netconn_delete(conn);
        return;
    }

    netconn_write(conn, json, strlen(json), NETCONN_COPY);
    netconn_write(conn, "\n", 1, NETCONN_NOCOPY);

    netconn_close(conn);
    netconn_delete(conn);
}
