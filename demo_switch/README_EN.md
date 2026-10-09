English | [中文](README.md)

# demo_switch — 3-Channel Smart Switch Firmware

3-channel relay switch firmware based on Ai-WB2 (BL602), supports WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.1.0**

## Hardware Connections

| Channel | GPIO | Description |
|---------|------|-------------|
| Switch 1 | 3 | Relay control (active high by default) |
| Switch 2 | 14 | Relay control |
| Switch 3 | 17 | Relay control |

- Relay polarity: `RELAY_ACTIVE_HIGH=1` (active high)
- Channel count: `SWITCH_COUNT=3`

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `switch_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns entity states |
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
{"mac":"AC:D8:29:7A:60:5D","name":"Smart Switch","model":"Ai-WB2-12F","sw_version":"1.1.0",
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
cd applications/home_assistant/demo_switch
make -j4
# Output: build_out/demo_switch.bin
```

## Configuration

All configuration in `demo_switch/demo_switch/app_config.h`:

```c
#define SWITCH_PINS        {3, 14, 17}       // GPIO for each channel
#define SWITCH_COUNT       3                  // Number of channels
#define RELAY_ACTIVE_HIGH  1                  // Active high
#define SWITCH_NAMES       {"Switch 1", "Switch 2", "Switch 3"}  // Channel names

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "Smart Switch"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "1.1.0"
```
