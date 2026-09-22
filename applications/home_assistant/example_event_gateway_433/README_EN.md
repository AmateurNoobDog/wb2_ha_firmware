English | [中文](README.md)

# example_event_gateway_433 — 433MHz Remote Control Gateway Firmware

433MHz remote receiver gateway firmware, supports WiFi + BLE BluFi provisioning + TCP JSON control + Push reporting.

**Current Version: 1.1.0**

## Hardware Connections

- Module: Ai-WB2-12F (BL602)
- 433MHz receiver: UART1 (TX=GPIO6, RX=GPIO4, 9600bps)
- Pair/Reset button: GPIO5

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `event_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns event state, push reports key events |
| `uart_433.c/h` | UART 433 receiver driver |
| `store.c/h` | WiFi credentials + push config persistence (EasyFlash) |
| `wifi_sta.c/h` | WiFi STA connection management |
| `blufi_app.h/c` | BLE BluFi provisioning module |
| `app_config.h` | Device configuration (pins/ports/names) |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"7C:B9:4C:D1:F7:67","name":"433 Gateway","model":"Ai-WB2-12F","sw_version":"1.1.0",
 "entities":[
   {"id":"7CB94CD1F767_001","type":"button","name":"Pair","icon":"mdi:remote","action":"pair"},
   {"id":"7CB94CD1F767_002","type":"button","name":"Reset","icon":"mdi:restore","action":"reset"},
   {"id":"7CB94CD1F767_003","type":"event","name":"Key Value","icon":"mdi:remote"}
 ]}
```

### Get State

```json
{"cmd":"get_state"}
```

Response (empty entities when no key event):
```json
{"state":"online","entities":[]}
```

With key event:
```json
{"state":"online","entities":[
  {"id":"7CB94CD1F767_003","type":"event","event_type":"release","event_id":"A2B861"}
]}
```

### Control Commands

```json
{"cmd":"pair"}            // Enter pairing mode
{"cmd":"reset"}           // Reset pairing
{"cmd":"push_cfg","ip":"192.168.1.100","port":9101}  // Configure push target
```

### Push (Port 9101)

Device actively pushes key events to HA's port 9101:

```json
{"id":"7CB94CD1F767_003","type":"event","event_type":"press","event_id":"A2B861"}
{"id":"7CB94CD1F767_003","type":"event","event_type":"release","event_id":"A2B861"}
```

## Build

```bash
cd applications/home_assistant/example_event_gateway_433
make -j4
# Output: build_out/example_event_gateway_433.bin
```

## Configuration

All configuration in `example_event_gateway_433/example_event_gateway_433/app_config.h`:

```c
#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "event"
#define DEVICE_NAME        "433 Gateway"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "1.1.0"
#define HA_PUSH_DEFAULT_PORT  9101
```

## Provisioning

1. Boot 3 times consecutively to enter BLE provisioning mode
2. Send WiFi credentials via Bluetooth connection
3. Device connects to WiFi after restart
