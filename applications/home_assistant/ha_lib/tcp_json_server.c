#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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

static void tcp_log(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (s_dev->log) {
        s_dev->log(buf);
    } else {
        blog_info("%s", buf);
    }
}

static void respond_device(struct netconn *conn)
{
    uint8_t mac[6];
    char line[512];
    char dev[384];
    int n;

    if (s_dev->get_device == NULL || s_dev->get_device(dev, sizeof(dev)) < 0) {
        dev[0] = '\0';
    }
    if (wifi_mgmr_sta_mac_get(mac) != 0) {
        memset(mac, 0, sizeof(mac));
    }
    n = snprintf(line, sizeof(line),
                 "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
                 "\"name\":\"%s\",\"model\":\"%s\",\"sw_version\":\"%s\",%s}",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                 s_dev->name, s_dev->model, s_dev->sw_version, dev);
    netconn_write(conn, line, n, NETCONN_NOCOPY);
    netconn_write(conn, "\r\n", 2, NETCONN_NOCOPY);
    tcp_log("[TCP] send get_device: %s", line);
}

static void respond_state(struct netconn *conn)
{
    char line[512];
    char dev[384];
    int n;

    if (s_dev->get_state == NULL || s_dev->get_state(dev, sizeof(dev)) < 0) {
        dev[0] = '\0';
    }
    n = snprintf(line, sizeof(line), "{\"state\":\"online\"%s%s}",
                 dev[0] ? "," : "", dev);
    netconn_write(conn, line, n, NETCONN_NOCOPY);
    netconn_write(conn, "\r\n", 2, NETCONN_NOCOPY);
    tcp_log("[TCP] send get_state: %s", line);
}

static void handle_request(struct netconn *conn, const char *buf)
{
    tcp_log("[TCP] recv: %s", buf);

    const char *cmd = ha_json_str(buf, "cmd");

    if (cmd == NULL) {
        return;
    }

    if (strcmp(cmd, "get_device") == 0) {
        respond_device(conn);
    } else if (strcmp(cmd, "get_state") == 0) {
        respond_state(conn);
    } else {
        if (s_dev->set_state != NULL) {
            if (s_dev->set_state(buf) != 0) {
                tcp_log("[TCP] set failed");
            }
        }
        respond_state(conn);
    }
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
        tcp_log("[TCP] netconn_new failed");
        return NULL;
    }

    if (netconn_bind(conn, IP_ADDR_ANY, s_dev->port) != ERR_OK) {
        tcp_log("[TCP] bind %d failed", s_dev->port);
        netconn_close(conn);
        netconn_delete(conn);
        return NULL;
    }

    if (netconn_listen(conn) != ERR_OK) {
        tcp_log("[TCP] listen failed");
        netconn_close(conn);
        netconn_delete(conn);
        return NULL;
    }

    tcp_log("[TCP] listening on port %d", s_dev->port);
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
            tcp_log("[TCP] accept err=%d, recreate listener", err);
            netconn_close(listen);
            netconn_delete(listen);
            while ((listen = server_listen()) == NULL) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
    vTaskDelete(NULL);
}
