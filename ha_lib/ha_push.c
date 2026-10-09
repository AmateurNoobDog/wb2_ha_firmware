#include <stdio.h>
#include <string.h>
#include <lwip/api.h>
#include <lwip/ip_addr.h>
#include "ha_push.h"
#include "ha_mdns_query.h"
#include "store.h"
#include "app_config.h"

static char s_ha_host[64];
static ip_addr_t s_ha_ip;
static uint8_t s_ip_resolved = 0;
static uint16_t s_ha_port = HA_PUSH_DEFAULT_PORT;
static uint8_t s_push_enabled = 0;

static void try_resolve(void)
{
    if (s_ha_host[0] == '\0') {
        return;
    }
    if (ha_resolve_host(s_ha_host, &s_ha_ip, 3000) == 0) {
        s_ip_resolved = 1;
        printf("[PUSH] resolved %s -> %s\n", s_ha_host, ipaddr_ntoa(&s_ha_ip));
    } else {
        s_ip_resolved = 0;
        printf("[PUSH] resolve failed for %s\n", s_ha_host);
    }
}

void ha_push_init(void)
{
    store_push_t cfg;
    if (store_push_load(&cfg) && cfg.enabled && cfg.host[0] != '\0') {
        ha_push_set_target(cfg.host, cfg.port);
        try_resolve();
        printf("[PUSH] loaded HA target: %s:%d\n", cfg.host, cfg.port);
    } else {
        printf("[PUSH] no HA target configured, push disabled\n");
    }
}

void ha_push_set_target(const char *host, uint16_t port)
{
    if (!host || host[0] == '\0') {
        return;
    }
    strncpy(s_ha_host, host, sizeof(s_ha_host) - 1);
    s_ha_host[sizeof(s_ha_host) - 1] = '\0';
    s_ha_port = (port > 0) ? port : HA_PUSH_DEFAULT_PORT;
    s_ip_resolved = 0;
    s_push_enabled = 1;
    store_push_save(host, s_ha_port);
}

uint8_t ha_push_enabled(void)
{
    return s_push_enabled;
}

void ha_push_clear(void)
{
    s_push_enabled = 0;
    memset(s_ha_host, 0, sizeof(s_ha_host));
    s_ip_resolved = 0;
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

    if (!s_ip_resolved) {
        try_resolve();
        if (!s_ip_resolved) {
            return;
        }
    }

    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL) {
        return;
    }

    netconn_set_recvtimeout(conn, 2000);

    err = netconn_connect(conn, &s_ha_ip, s_ha_port);
    if (err != ERR_OK) {
        netconn_delete(conn);
        s_ip_resolved = 0;
        return;
    }

    netconn_write(conn, json, strlen(json), NETCONN_COPY);
    netconn_write(conn, "\n", 1, NETCONN_NOCOPY);

    netconn_close(conn);
    netconn_delete(conn);
}
