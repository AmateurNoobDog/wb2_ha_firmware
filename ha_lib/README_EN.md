English | [中文](README.md)

# ha_lib — Home Assistant Shared Library

Device-agnostic TCP JSON server library, reused by all HA firmware projects (light/switch/radar_rd_01/433_gateway/key_sensor/usb_sw).

## File Description

| File | Description |
|------|-------------|
| `ha_device.h` | Device abstraction struct `ha_device_t`, defines device type/name/model/version/port/callbacks |
| `ha_json.c/h` | cJSON-based JSON field parser (`ha_json_str`/`ha_json_int`) |
| `tcp_json_server.c` | TCP server main logic: listens on port, responds to `get_device`/`get_state`, dispatches `set` |
| `ha_push.c/h` | Push module (NVS config target IP/port + TCP push) |
| `bouffalo.mk` | BL602 SDK component build script |

## API

### ha_device_t (Device Abstraction)

```c
typedef struct {
    const char *type;                     // Device type, e.g. "light"/"switch"/"radar"/"event"
    const char *name;                     // Device name, e.g. "Color Light"/"Smart Switch"/"433 Gateway"
    const char *model;                    // Device model, e.g. "Ai-WB2-12F"/"RD-01"
    const char *sw_version;               // Firmware version, e.g. "1.0.1"
    int port;                             // TCP listening port, default 9100
    int (*get_device)(char *buf, int buf_len);  // Fill entity definitions (without braces)
    int (*get_state)(char *buf, int buf_len);   // Fill entity states (without braces)
    int (*set_state)(const char *cmd_json);     // Parse and execute set command
    void (*log)(const char *msg);               // Optional: log output callback
} ha_device_t;
```

### ha_tcp_server_start()

```c
void ha_tcp_server_start(void *pvParameters);
```

TCP server entry, pvParameters must point to a persistent `const ha_device_t`.
Runs as a FreeRTOS task with default stack size 4096.

Workflow:
1. Listens on `ha_device_t.port`
2. When receiving `{"cmd":"get_device"}`, calls `get_device()` to return entity definitions
3. When receiving `{"cmd":"get_state"}`, calls `get_state()` to return entity states
4. When receiving `{"cmd":"set",...}`, calls `set_state()` to execute control

### ha_json_str / ha_json_int

```c
const char *ha_json_str(const char *buf, const char *key);
int ha_json_int(const char *buf, const char *key, int def);
```

JSON field extraction based on cJSON library, used for parsing commands.
`ha_json_str` returns null-terminated strings, safe for `strcmp`.

### ha_push Module

```c
void ha_push_init(void);                              // Load config from NVS
void ha_push_set_target(const char *ip, uint16_t port); // Set target and save to NVS
void ha_push_send(const char *json);                  // Send JSON to HA (non-blocking)
uint8_t ha_push_enabled(void);                        // Whether target is configured
```

## New Device Integration Steps

1. Define `DEVICE_TYPE`/`DEVICE_NAME`/`DEVICE_MODEL`/`DEVICE_SW_VERSION`/`TCP_SERVER_PORT` in `app_config.h`
2. Implement `xxx_handler_get_device()`, `xxx_handler_get_state()`, and `xxx_handler_set_state()` functions
3. Register `ha_device_t` in `main.c` and start the server:

```c
static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .model = DEVICE_MODEL,
    .sw_version = DEVICE_SW_VERSION,
    .port = TCP_SERVER_PORT,
    .get_device = xxx_handler_get_device,
    .get_state = xxx_handler_get_state,
    .set_state = xxx_handler_set_state,
};

// Start after WiFi gets IP
xTaskCreate(ha_tcp_server_start, "tcp_json", TCP_SERVER_STACK, (void *)&ha_dev, 15, NULL);
```

## Protocol Format (v2)

### get_device — Return Entity Definitions

```
Request: {"cmd":"get_device"}
Response: {"mac":"XX:XX:XX:XX:XX:XX","name":"...","model":"...","sw_version":"...",
           "entities":[{"id":"...","type":"...","name":"...","icon":"..."}]}
```

### get_state — Return Entity States

```
Request: {"cmd":"get_state"}
Response: {"state":"online","entities":[{"id":"...","type":"...","on":1}]}
```

### set — Control Entity

```
Request: {"cmd":"set","id":"XXX_NNN",...}
Response: {"state":"online","entities":[...]}
```

## Entity Types

| Type | Has Command | Has State | Description |
|------|-------------|-----------|-------------|
| `light` | set | Yes | RGB light |
| `switch` | set | Yes | Relay switch |
| `binary_sensor` | — | Yes | Binary sensor |
| `button` | action | — | Button |
| `event` | — | Yes | Event |
| `sensor` | — | Yes | Data sensor |
