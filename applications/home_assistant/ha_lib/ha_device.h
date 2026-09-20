#ifndef __HA_DEVICE_H__
#define __HA_DEVICE_H__

/*
 * Device abstraction used by the Home Assistant TCP server.
 *
 * A project implements a device handler (get_state/set_state) and registers
 * it via ha_tcp_server_start(). The server is device-agnostic: it only
 * handles the TCP/JSON transport and the common fields (mac/type/name).
 */

typedef struct {
    const char *type;                     /* device type reported to HA, e.g. "wb2" */
    const char *name;                     /* device name reported to HA, e.g. "彩灯" */
    const char *model;                    /* device model, e.g. "Ai-WB2-12F" */
    const char *manufacturer;             /* manufacturer, e.g. "AND-DIY" */
    const char *sw_version;               /* firmware version, e.g. "0.10.0" */
    int port;                             /* TCP listen port, e.g. 9100 */
    int (*get_device)(char *buf, int buf_len);  /* fill device info + entity definitions (no outer braces) */
    int (*get_state)(char *buf, int buf_len);   /* fill entity state fields only (no braces) */
    int (*set_state)(const char *cmd_json);     /* parse and apply a "cmd" request */
    void (*log)(const char *msg);               /* project-specific log output, e.g. uart_433_log */
} ha_device_t;

/*
 * Task entry. pvParameters must point to a const ha_device_t that stays
 * valid for the lifetime of the server task.
 */
void ha_tcp_server_start(void *pvParameters);

#endif /* __HA_DEVICE_H__ */