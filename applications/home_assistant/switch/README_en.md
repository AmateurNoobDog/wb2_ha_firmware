# switch — 3-Channel Smart Switch Firmware

> **English** | [中文](README.md)

3-channel relay switch firmware based on Ai-WB2 (BL602), with WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.0.1**

## Hardware Connection

| Channel | GPIO | Description |
|---------|------|-------------|
| Switch 1 | 3 | Relay control (active-high by default) |
| Switch 2 | 14 | Relay control |
| Switch 3 | 17 | Relay control |

- Relay polarity: `RELAY_ACTIVE_HIGH=1` (active-high)
- Channel count: `SWITCH_COUNT=3`

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `switch_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns entity states |
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
{"mac":"AC:D8:29:7A:60:5D","name":"Smart Switch","model":"Ai-WB2-12F","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"switch","name":"Switch 1","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_002","type":"switch","name":"Switch 2","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_003","type":"switch","name":"Switch 3","icon":"mdi:toggle-switch"}
 ]}
```

### Get State

```json
{"cmd":"get_state"}
```

Response:
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":0},
  {"id":"ACD8297A605D_002","type":"switch","on":1},
  {"id":"ACD8297A605D_003","type":"switch","on":0}
]}
```

### Control

```json
{"cmd":"set","id":"ACD8297A605D_001","on":1}       // Turn on switch 1
{"cmd":"set","id":"ACD8297A605D_002","on":0}       // Turn off switch 2
```

### Push

Device connects to HA's port 9101 and pushes all switch states:
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":1},
  {"id":"ACD8297A605D_002","type":"switch","on":0},
  {"id":"ACD8297A605D_003","type":"switch","on":0}
]}
```

## Build

```bash
cd applications/home_assistant/switch
make -j4
# Output: build_out/switch.bin
```

## Configuration

All config in `switch/switch/app_config.h`:

```c
#define SWITCH_PINS        {3, 14, 17}       // Channel GPIOs
#define SWITCH_COUNT       3                  // Channel count
#define RELAY_ACTIVE_HIGH  1                  // Active-high
#define SWITCH_NAMES       {"Switch 1", "Switch 2", "Switch 3"}  // Channel names

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "Smart Switch"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "1.0.1"
```
