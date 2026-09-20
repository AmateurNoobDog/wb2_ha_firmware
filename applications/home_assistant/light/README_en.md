# light — RGB LED Firmware

> **English** | [中文](README.md)

RGB LED firmware based on Ai-WB2 (BL602), with WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.0.1**

## Hardware Connection

| Function | GPIO | PWM Channel | Description |
|----------|------|-------------|-------------|
| LED Red | 14 | CH4 | PWM output |
| LED Green | 17 | CH2 | PWM output |
| LED Blue | 3 | CH3 | PWM output |

- PWM frequency: 5000Hz
- RGB range: 0-255

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `led_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns RGB state |
| `led.c/h` | LED low-level driver (PWM init/set/read) |
| `app_config.h` | Device config (pins/port/name) |
| `bouffalo.mk` | BL602 SDK component build script |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"AC:D8:29:7A:60:5D","name":"LED Light","model":"Ai-WB2-12F","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"light","name":"LED Light","icon":"mdi:lightbulb"}
 ]}
```

### Get State

```json
{"cmd":"get_state"}
```

Response:
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"light","r":255,"g":128,"b":0,"brightness":255}
]}
```

### Control

```json
{"cmd":"set","id":"ACD8297A605D_001","r":255,"g":0,"b":128}          // Set color
{"cmd":"set","id":"ACD8297A605D_001","r":255,"g":0,"b":0,"brightness":128}  // Set color and brightness
```

### Push

Device connects to HA's port 9101 and pushes light state:
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"light","r":255,"g":128,"b":0,"brightness":255}
]}
```

## Build

```bash
cd applications/home_assistant/light
make -j4
# Output: build_out/light.bin
```

## Configuration

All config in `light/light/app_config.h`:

```c
#define LED_RED_PIN        14
#define LED_GREEN_PIN      17
#define LED_BLUE_PIN       3
#define LED_RED_CH         4
#define LED_GREEN_CH       2
#define LED_BLUE_CH        3
#define LED_PWM_FREQ       5000

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "light"
#define DEVICE_NAME        "LED Light"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "1.0.1"
```
