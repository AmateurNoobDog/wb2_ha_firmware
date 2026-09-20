# ha_lib — Home Assistant Shared Library

> **English** | [中文](README.md)

Device-agnostic TCP JSON server library, reused by all HA firmware projects (light/switch/radar_rd_01/433_gateway/usb_sw/tts).

## File Description

| File | Description |
|------|-------------|
| `ha_device.h` | Device abstraction struct `ha_device_t`, defines type/name/model/manufacturer/version/port/callbacks |
| `ha_json.c/h` | cJSON-based JSON field parser (`ha_json_str`/`ha_json_int`) |
| `tcp_json_server.c` | TCP server main logic: listen port, respond to `get_device`/`get_state`, dispatch `set` |
| `ha_push.c/h` | Push module (NVS config for target host/port + TCP push) |
| `ha_mdns_query.c/h` | Hostname resolution (.local via mDNS multicast query, others via DNS, with 4-entry cache) |
| `bouffalo.mk` | BL602 SDK component build script |

## API

### ha_device_t (Device Abstraction)

```c
typedef struct {
    const char *type;                     // Device type, e.g. "light"/"switch"/"radar"/"event"/"tts"
    const char *name;                     // Device name, e.g. "LED Light"/"Smart Switch"/"433 Gateway"
    const char *model;                    // Device model, e.g. "Ai-WB2-12F"/"RD-01"
    const char *manufacturer;             // Manufacturer, e.g. "AND-DIY"
    const char *sw_version;               // Firmware version, e.g. "1.0.1"
    int port;                             // TCP listen port, e.g. 9100
    int (*get_device)(char *buf, int buf_len);  // Fill entity definitions (without outer braces)
    int (*get_state)(char *buf, int buf_len);   // Fill entity state fields only (without braces)
    int (*set_state)(const char *cmd_json);     // Parse and apply a "cmd" request
    void (*log)(const char *msg);               // Optional: log output callback
} ha_device_t;
```

### ha_tcp_server_start()

```c
void ha_tcp_server_start(void *pvParameters);
```

TCP server entry. `pvParameters` must point to a persistent `const ha_device_t`.
Runs as a FreeRTOS task, default stack size 4096.

Workflow:
1. Listen on `ha_device_t.port`
2. On `{"cmd":"get_device"}` → call `get_device()` to return entity definitions
3. On `{"cmd":"get_state"}` → call `get_state()` to return entity states
4. On `{"cmd":"set",...}` → call `set_state()` to execute control

### ha_json_str / ha_json_int

```c
const char *ha_json_str(const char *buf, const char *key);
int ha_json_int(const char *buf, const char *key, int def);
```

JSON field extraction based on cJSON library, for parsing commands.
`ha_json_str` returns null-terminated strings, safe for `strcmp`.

### ha_push Module

```c
void ha_push_init(void);                                    // Load config from NVS
void ha_push_set_target(const char *host, uint16_t port);   // Set target host/IP and save to NVS
void ha_push_clear(void);                                   // Clear push config and disable push
void ha_push_send(const char *json);                        // Send JSON to HA (blocking, synchronous TCP)
uint8_t ha_push_enabled(void);                              // Whether target is configured
```

The `host` parameter of `ha_push_set_target` supports IP addresses (e.g. `"192.168.1.100"`) and hostnames (e.g. `"homeassistant.local"`).
Hostnames are automatically resolved via the `ha_mdns_query` module (.local via mDNS, others via DNS).

## New Device Integration Steps

1. Define `DEVICE_TYPE`/`DEVICE_NAME`/`DEVICE_MODEL`/`DEVICE_MANUFACTURER`/`DEVICE_SW_VERSION`/`TCP_SERVER_PORT` in `app_config.h`
2. Implement `xxx_handler_get_device()`, `xxx_handler_get_state()`, and `xxx_handler_set_state()` functions
3. Register `ha_device_t` in `main.c` and start the server:

```c
static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .model = DEVICE_MODEL,
    .manufacturer = DEVICE_MANUFACTURER,
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
Request:  {"cmd":"get_device"}
Response: {"mac":"XX:XX:XX:XX:XX:XX","name":"...","model":"...","manufacturer":"...","sw_version":"...",
           "entities":[{"id":"...","type":"...","name":"...","icon":"..."}]}
```

### get_state — Return Entity States

```
Request:  {"cmd":"get_state"}
Response: {"state":"online","entities":[{"id":"...","type":"...","on":1}]}
```

### set — Control Entity

```
Request:  {"cmd":"set","id":"XXX_NNN",...}
Response: {"state":"online","entities":[...]}
```

## Entity Types

| Type | Has command | Has state | Description |
|------|-----------|----------|-------------|
| `light` | set | Yes | RGB LED |
| `switch` | set | Yes | Relay switch |
| `binary_sensor` | — | Yes | Binary sensor |
| `button` | action | — | Button |
| `event` | — | Yes | Event |
| `sensor` | — | Yes | Data sensor |
| `notify` | set (text) | — | Notification (TTS speech synthesis) |
| `number` | set (value) | Yes | Numeric (TTS volume/speed) |
