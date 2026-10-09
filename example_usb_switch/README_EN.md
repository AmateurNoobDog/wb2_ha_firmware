English | [中文](README.md)

# example_usb_switch — Single-Channel USB Power Switch Firmware

Single-channel USB power switch firmware based on Ai-WB2 (BL602), supports WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.1.0**

## Hardware Connections

| Channel | GPIO | Description |
|---------|------|-------------|
| USB Power | 4 | Relay control (active low) |

- Relay polarity: `RELAY_ACTIVE_HIGH=0` (active low)
- Channel count: `SWITCH_COUNT=1`

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `switch_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns switch state |
| `relay.c/h` | Relay low-level driver (GPIO init/set/read) |
| `store.c/h` | WiFi credential persistence (EasyFlash) |
| `wifi_sta.c/h` | WiFi STA connection management |
| `blufi_app.c/h` | BLE BluFi provisioning module |
| `app_config.h` | Device configuration (pins/ports/names/polarity) |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"7C:B9:4C:D4:5B:0B","name":"USB Power Switch","model":"Ai-WB2-01S","sw_version":"1.1.0",
 "entities":[
   {"id":"7CB94CD45B0B_001","type":"switch","name":"USB","icon":"mdi:toggle-switch"}
 ]}
```

### Get State

```json
{"cmd":"get_state"}
```

Response:
```json
{"state":"online","entities":[
  {"id":"7CB94CD45B0B_001","type":"switch","on":0}
]}
```

### Control

```json
{"cmd":"set","id":"7CB94CD45B0B_001","on":1}       // Turn on
{"cmd":"set","id":"7CB94CD45B0B_001","on":0}       // Turn off
```

### Push

Device connects to HA's port 9101 and pushes switch state:
```json
{"entities":[
  {"id":"7CB94CD45B0B_001","type":"switch","on":1}
]}
```

## Build

```bash
cd applications/home_assistant/example_usb_switch
make -j4
# Output: build_out/example_usb_switch.bin
```

## Configuration

All configuration in `example_usb_switch/example_usb_switch/app_config.h`:

```c
#define SWITCH_PINS        {4}              // Relay GPIO
#define SWITCH_COUNT       1                // Number of channels
#define RELAY_ACTIVE_HIGH  0                // Active low
#define SWITCH_NAMES       {"USB"}          // Channel name

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "USB Power Switch"
#define DEVICE_MODEL       "Ai-WB2-01S"
#define DEVICE_SW_VERSION  "1.1.0"
```

## Provisioning

1. Boot 3 times consecutively to enter BLE provisioning mode
2. Send WiFi credentials via Bluetooth connection
3. Device connects to WiFi after restart
