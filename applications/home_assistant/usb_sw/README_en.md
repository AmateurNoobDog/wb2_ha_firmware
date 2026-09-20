# usb_sw — Single USB Switch Firmware

> **English** | [中文](README.md)

Single USB switch firmware based on Ai-WB2 (BL602), with WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.0.1**

## Hardware Connection

| Channel | GPIO | Description |
|---------|------|-------------|
| USB Switch | 4 | Relay control (active-low) |

- Relay polarity: `RELAY_ACTIVE_HIGH=0` (active-low)
- Channel count: `SWITCH_COUNT=1`

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `switch_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns switch state |
| `relay.c/h` | Relay low-level driver (GPIO init/set/read) |
| `app_config.h` | Device config (pins/port/name/polarity) |
| `bouffalo.mk` | BL602 SDK component build script |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"7C:B9:4C:D4:5B:0B","name":"USB Switch","model":"Ai-WB2-01S","sw_version":"1.0.1",
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
cd applications/home_assistant/usb_sw
make -j4
# Output: build_out/usb_sw.bin
```

## Configuration

All config in `usb_sw/usb_sw/app_config.h`:

```c
#define SWITCH_PINS        {4}              // Relay GPIO
#define SWITCH_COUNT       1                // Channel count
#define RELAY_ACTIVE_HIGH  0                // Active-low
#define SWITCH_NAMES       {"USB"}          // Channel name

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "USB Switch"
#define DEVICE_MODEL       "Ai-WB2-01S"
#define DEVICE_SW_VERSION  "1.0.1"
```

## WiFi Provisioning

1. Power on 3 times consecutively to enter BLE provisioning mode
2. Connect via Bluetooth and send WiFi credentials
3. Device restarts and connects to WiFi
