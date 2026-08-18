#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lwip/api.h>
#include "blog.h"
#include "ha_device.h"
#include "ha_json.h"
#include "wifi_mgmr_ext.h"

#define BUF_LEN         256
#define RECV_TIMEOUT_MS 5000

static const ha_device_t *s_dev;

static void respond_state(struct netconn *conn)
{
    uint8_t mac[6];
    char line[256];
    char dev[128];
    int n;

    if (s_dev->get_state(dev, sizeof(dev)) < 0) {
        dev[0] = '\0';
    }
    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    n = snprintf(line, sizeof(line),
                 "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
                 "\"type\":\"%s\",\"name\":\"%s\",%s}",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                 s_dev->type, s_dev->name, dev);
    netconn_write(conn, line, n, NETCONN_NOCOPY);
    netconn_write(conn, "\r\n", 2, NETCONN_NOCOPY);
}

static void handle_request(struct netconn *conn, const char *buf)
{
    const char *cmd = ha_json_str(buf, "cmd");

    if (cmd != NULL && strncmp(cmd, "set", 3) == 0) {
        if (s_dev->set_state(buf) != 0) {
            blog_info("[TCP] set failed");
        }
    }

    respond_state(conn);
}

static void handle_conn(struct netconn *conn)
{
    char line[BUF_LEN];
    int linelen = 0;
    err_t err;

    netconn_set_recvtimeout(conn, RECV_TIMEOUT_MS);

    for (;;) {
        struct netbuf *recvbuf;

        err = netconn_recv(conn, &recvbuf);
        if (err != ERR_OK) {
            break;
        }

        if (recvbuf != NULL) {
            char chunk[BUF_LEN];
            u16_t tot = netbuf_len(recvbuf);
            u16_t i;

            if (tot > BUF_LEN) {
                tot = BUF_LEN;
            }
            netbuf_copy(recvbuf, chunk, tot);

            for (i = 0; i < tot; i++) {
                char c = chunk[i];

                if (c == '\n') {
                    if (linelen > 0) {
                        line[linelen] = '\0';
                        while (linelen > 0 &&
                               (line[linelen - 1] == '\r' ||
                                line[linelen - 1] == ' ' ||
                                line[linelen - 1] == '\t')) {
                            line[--linelen] = '\0';
                        }
                        if (linelen > 0) {
                            handle_request(conn, line);
                        }
                        linelen = 0;
                    }
                } else if (linelen < BUF_LEN - 1) {
                    line[linelen++] = c;
                }
            }
            netbuf_delete(recvbuf);
        }
    }

    netconn_close(conn);
    netconn_delete(conn);
}

static struct netconn *server_listen(void)
{
    struct netconn *conn;

    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL) {
        blog_info("[TCP] netconn_new failed");
        return NULL;
    }

    if (netconn_bind(conn, IP_ADDR_ANY, s_dev->port) != ERR_OK) {
        blog_info("[TCP] bind %d failed", s_dev->port);
        netconn_close(conn);
        netconn_delete(conn);
        return NULL;
    }

    if (netconn_listen(conn) != ERR_OK) {
        blog_info("[TCP] listen failed");
        netconn_close(conn);
        netconn_delete(conn);
        return NULL;
    }

    blog_info("[TCP] listening on port %d", s_dev->port);
    return conn;
}

void ha_tcp_server_start(void *pvParameters)
{
    struct netconn *listen;
    struct netconn *client;
    err_t err;

    s_dev = (const ha_device_t *)pvParameters;

    while ((listen = server_listen()) == NULL) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    for (;;) {
        err = netconn_accept(listen, &client);
        if (err == ERR_OK) {
            handle_conn(client);
        } else {
            blog_info("[TCP] accept err=%d, recreate listener", err);
            netconn_close(listen);
            netconn_delete(listen);
            while ((listen = server_listen()) == NULL) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
    vTaskDelete(NULL);
}